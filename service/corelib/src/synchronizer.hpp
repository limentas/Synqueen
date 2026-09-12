#pragma once

#include "foldermanager.hpp"
#include "patch/patchbackend.hpp"
#include "settings.hpp"
#include "uigate/foldermanagementgrpc.hpp"
#include "uigate/iuigateprovider.hpp"
#include "utils/corralheader.hpp"
#include "utils/uvutils.hpp"
#include "watchers/timerwatcher.hpp"

#include <functional>
#include <vector>

namespace synqueen {

class Synchronizer : public IUiGateProvider {
public:
  Synchronizer(uv_loop_t *loop);
  ~Synchronizer();

  corral::Task<void> run(corral::TaskStarted<> started = {});
  void shutdown();

  void loadSettings(const Settings &settings,
                    std::function<void(const Settings &)> saveSettingsFunc);

  void checkAllLocal();
  void checkAllRemotes();

  // IUiGateProvider interface
  virtual IUiGateProvider::ListFolders listFolders() const override;
  virtual corral::Task<void> addFolder(const Folder &folder) override;
  virtual corral::Task<void>
  removeFolder(const std::filesystem::path &path) override;

private:
  void saveSettings();
  void initFolders();
  uv_async_t *createAsyncEvent(uv_loop_t *loop, uv_async_cb callback);

private:
  Settings settings;
  std::function<void(const Settings &)> saveSettingsFunc;
  uv_loop_t *loop = nullptr;
  std::unique_ptr<PatchBackend> patchBackend;
  std::vector<FolderManagerPtr> folderManagers;
  // We have one timer watcher for all folders
  std::unique_ptr<TimerWatcher> timerWatcher;
  std::unique_ptr<FolderManagementGrpc> folderManagementGrpc;

  SharedAsyncPtr checkLocalEvent;
  SharedAsyncPtr checkRemoteEvent;

  corral::Nursery *nursery = nullptr;
};

} // namespace synqueen
