#include "synchronizer.hpp"

#include "patch/patchbackend.hpp"
#include "utils/corralheader.hpp"
#include "utils/logger.hpp"
#include "utils/taskscheduler.hpp"
#include "utils/uuid.hpp"

#include <filesystem>
#include <memory>

using namespace std;

namespace synqueen {

using namespace patch;
using namespace cloud;

Synchronizer::Synchronizer(uv_loop_t *loop)
    : loop(loop), patchBackend(createPatchBackend(loop)),
      folderManagementGrpc(std::make_unique<FolderManagementGrpc>(*this)) {}

Synchronizer::~Synchronizer() {
  // Make sure that corral does what is should
  assert(nursery == nullptr);
}

corral::Task<void> Synchronizer::run(corral::TaskStarted<> started) {
  checkLocalEvent = SharedAsyncPtr(
      createAsyncEvent(loop,
                       [](uv_async_t *handle) {
                         auto synchronizer =
                             reinterpret_cast<Synchronizer *>(handle->data);
                         synchronizer->checkAllLocal();
                       }),
      deleteHandle<uv_async_t>);
  checkRemoteEvent = SharedAsyncPtr(
      createAsyncEvent(loop,
                       [](uv_async_t *handle) {
                         auto synchronizer =
                             reinterpret_cast<Synchronizer *>(handle->data);
                         synchronizer->checkAllRemotes();
                       }),
      deleteHandle<uv_async_t>);
  timerWatcher =
      std::make_unique<TimerWatcher>(checkLocalEvent, checkRemoteEvent, loop);
  timerWatcher->startWatch();

  // The nursery will be cleared upon last task completion/cancellation
  CORRAL_WITH_NURSERY(n) {
    co_await n.start(corral::openNursery, std::ref(nursery));
    TaskScheduler::initialize(loop, nursery);
    loadFolders();
    started(); // signal readiness
    co_return corral::join;
  };

  // We arrive here after all corral tasks have completed or cancelled

  // NOTE: The approach here can be fragile. In worst case scenario loop may
  // never exit if one of handles is not closed properly.
  // At this moment I don't know how painful this approach will be.
  // If it doesn't work - consider using `uvw` C++ wrapper for libuv.
  // It keeps track of all handles and can close them properly.

  // We just have to close all libuv handles and the loop will exit
  // automatically
  timerWatcher.reset();
  checkLocalEvent.reset();
  checkRemoteEvent.reset();
}

void Synchronizer::shutdown() {
  assert(nursery != nullptr);
  assert(timerWatcher != nullptr);
  timerWatcher->stopWatch();
  nursery->start([this]() -> corral::Task<void> {
    co_await corral::noncancellable(patchBackend->shutdown());
  });
  nursery->cancel();
}

// Must be called before run()
void Synchronizer::loadSettings(
    const Settings &settings,
    std::function<void(const Settings &)> saveSettingsFunc) {
  this->settings = settings;
  this->saveSettingsFunc = saveSettingsFunc;

  for (const auto &folderSettings : settings.folders) {
    // TODO: set correct CloudConfig
    auto patchStorage = std::make_unique<PatchStorage>(folderSettings.path);
    auto folderManager = std::make_unique<FolderManager>(
        folderSettings.id, folderSettings.path, *patchBackend, *patchStorage);
    std::list<CloudDestinationPtr> cloudDestinations;
    for (const auto &config : folderSettings.cloudDestinations) {
      cloudDestinations.push_back(
          std::make_unique<CloudDestination>(*patchStorage));
    }
    auto folder = FolderStruct{
        .path = folderSettings.path,
        .patchStorage = std::move(patchStorage),
        .folderManager = std::move(folderManager),
        .cloudDestinations = std::move(cloudDestinations),
    };

    folders.push_back(std::move(folder));
    SPDLOG_INFO("Loaded folder manager for path: {}", folderSettings.path);
  }
}

void Synchronizer::checkAllLocal() {
  SPDLOG_INFO("Checking all local folder states...");
  for (const auto &folder : folders) {
    folder.folderManager->synchronize();
  }
}

void Synchronizer::checkAllRemotes() {
  SPDLOG_INFO("Checking all cloud states...");
  for (const auto &folder : folders) {
    for (const auto &cloudDestination : folder.cloudDestinations) {
      cloudDestination->synchronize();
    }
  }
}

IUiGateProvider::ListFolders Synchronizer::listFolders() const {
  IUiGateProvider::ListFolders result;
  for (const auto &f : this->settings.folders) {
    // TODO: set cloudConfig
    result.push_back(Folder{f.path});
  }
  return result;
}

corral::Task<void> Synchronizer::addFolder(const Folder &folder) {
  SPDLOG_INFO("Add folder requested: {}", folder.path);
  try {
    co_await patchBackend->initRepoFolder(folder.path);
  } catch (const std::exception &e) {
    SPDLOG_ERROR("Failed to initialize repo folder {}: {}",
                 folder.path.string(), e.what());
    throw;
  }

  auto id = generateUUID();
  // auto fm = std::make_shared<FolderManager>(id, folder.path, *patchBackend,
  //                                          std::move(cloudProvider));
  // fm->setupState(*nursery);
  // folderManagers.push_back(fm);
  // TODO: set cloudConfig for the new folder
  this->settings.folders.push_back(
      FolderSettings{.id = id, .path = folder.path});
  saveSettings();

  auto patchStorage = std::make_unique<PatchStorage>(folder.path);
  auto folderManager = std::make_unique<FolderManager>(
      id, folder.path, *patchBackend, *patchStorage);
  folderManager->setupState(*nursery);
  std::list<CloudDestinationPtr> cloudDestinations;
  // TODO: handle cloud destinations for the new folder
  auto f = FolderStruct{
      .path = folder.path,
      .patchStorage = std::move(patchStorage),
      .folderManager = std::move(folderManager),
      .cloudDestinations = std::move(cloudDestinations),
  };

  folders.push_back(std::move(f));
  co_return;
}

corral::Task<void>
Synchronizer::removeFolder(const std::filesystem::path &path) {
  co_return;
}

void Synchronizer::saveSettings() {
  if (!saveSettingsFunc)
    return;
  saveSettingsFunc(settings);
}

void Synchronizer::loadFolders() {
  // We set nursery for folder managers
  for (const auto &folder : folders) {
    // TODO: handle possible loading errors. We could mark the folder as broken
    // or reinitialize it.
    folder.folderManager->loadState(*nursery);
  }

  folderManagementGrpc->initialize(*nursery);
}

uv_async_t *Synchronizer::createAsyncEvent(uv_loop_t *loop,
                                           uv_async_cb callback) {
  auto event = new uv_async_t();
  auto result = uv_async_init(loop, event, callback);
  if (result < 0) {
    delete event;
    throw std::runtime_error("Failed to create async event. Error: " +
                             std::string(uv_strerror(result)));
  }
  event->data = this;
  return event;
}

} // namespace synqueen
