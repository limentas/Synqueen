#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <thread>

#include "corelib/include/utils/uvutils.hpp"
#include "corelib/src/patch/patchbackend.hpp"
#include "corelib/src/patch/patchexchange.hpp"

using namespace testing;
using namespace std;
using namespace synqueen;
using namespace std::chrono_literals;

TEST(PatchExchangeTest, CtorDtor) {
  uv_loop_t loop;
  uv_loop_init(&loop);
  {
    auto model = make_unique<PatchExchange>(
        std::unique_ptr<PatchBackend>(createPatchBackend(&loop)));
    uv_run(&loop, UV_RUN_DEFAULT);
  }
  uv_loop_close(&loop);
}

class MockPatchBackend : public PatchBackend {
public:
  MOCK_METHOD(corral::Task<patch::LocalStateResult>, checkLocalState,
              (const std::string &folderPath,
               const patch::LocalStateCallbackPtr &callback),
              (override));
  MOCK_METHOD(void, preparePatch,
              (const std::string &folderPath,
               const patch::PreparePatchCallbackPtr &callback),
              (override));
};

TEST(PatchExchangeTest, CheckCommandsQueuing) {
  uv_loop_t loop;
  uv_loop_init(&loop);
  {
    auto backend = new MockPatchBackend();
    auto model =
        make_unique<PatchExchange>(std::unique_ptr<PatchBackend>(backend));
    auto checkLocalResult = new uv_async_t();
    auto preparePatchResult = new uv_async_t();
    std::thread *thread = nullptr;

    struct CheckData {
      std::string folderPath;
      patch::LocalStateCallbackPtr callback;
      std::thread *&thread;
    };
    struct PrepareData {
      std::string folderPath;
      patch::PreparePatchCallbackPtr callback;
      uv_async_t *checkLocalResult;
      uv_async_t *preparePatchResult;
      std::thread *&thread;
    };
    EXPECT_EQ(uv_async_init(&loop, checkLocalResult,
                            [](uv_async_t *handle) {
                              auto data =
                                  reinterpret_cast<CheckData *>(handle->data);
                              (*data->callback)(
                                  data->folderPath,
                                  patch::LocalStateResult{.initialized = true});
                              if (data->thread) {
                                data->thread->join();
                                delete data->thread;
                                data->thread = nullptr;
                              }
                              delete data;
                            }),
              0);
    EXPECT_EQ(uv_async_init(&loop, preparePatchResult,
                            [](uv_async_t *handle) {
                              auto data =
                                  reinterpret_cast<PrepareData *>(handle->data);
                              (*data->callback)(data->folderPath,
                                                patch::PreparePatchResult{});
                              if (data->folderPath == "folder2") {
                                // Stop the loop after the last callback is
                                // called
                                deleteHandle(data->checkLocalResult);
                                deleteHandle(data->preparePatchResult);
                                printLoopHandles(handle->loop);
                              }
                              if (data->thread) {
                                data->thread->join();
                                delete data->thread;
                                data->thread = nullptr;
                              }
                              delete data;
                            }),
              0);

    std::atomic_bool commandRunning = false;
    ON_CALL(*backend, checkLocalState(_, _))
        .WillByDefault([checkLocalResult, &commandRunning,
                        &thread](const std::string &folderPath,
                                 const patch::LocalStateCallbackPtr &callback) {
          EXPECT_FALSE(commandRunning);
          commandRunning = true;
          // Simulate async behavior by calling the callback after a short
          // delay
          spdlog::info("checkLocalState called for folder: {}", folderPath);
          checkLocalResult->data = new CheckData{folderPath, callback, thread};
          thread = new std::thread([checkLocalResult, &commandRunning]() {
            std::this_thread::sleep_for(20ms);
            commandRunning = false;
            uv_async_send(checkLocalResult);
          });
          // TODO:
          return corral::Task<patch::LocalStateResult>();
        });
    ON_CALL(*backend, preparePatch(_, _))
        .WillByDefault(
            [preparePatchResult, &commandRunning, checkLocalResult,
             &thread](const std::string &folderPath,
                      const patch::PreparePatchCallbackPtr &callback) {
              EXPECT_FALSE(commandRunning);
              commandRunning = true;
              // Simulate async behavior by calling the callback after a short
              // delay
              spdlog::info("preparePatch called for folder: {}", folderPath);
              preparePatchResult->data =
                  new PrepareData{folderPath, callback, checkLocalResult,
                                  preparePatchResult, thread};
              thread = new std::thread([preparePatchResult, &commandRunning]() {
                std::this_thread::sleep_for(20ms);
                commandRunning = false;
                uv_async_send(preparePatchResult);
              });
            });
    EXPECT_CALL(*backend, checkLocalState("folder1", _)).Times(1);
    EXPECT_CALL(*backend, checkLocalState("folder2", _)).Times(1);
    EXPECT_CALL(*backend, preparePatch("folder1", _)).Times(1);
    EXPECT_CALL(*backend, preparePatch("folder2", _)).Times(1);

    model->checkLocalState("folder1",
                           make_shared<patch::LocalStateCallback>(
                               [](const std::string &folderPath,
                                  const patch::LocalStateResult &result) {
                                 // Check that possible exceptions in callbacks
                                 // are handled gracefully
                                 throw std::runtime_error("Test error");
                               }));
    model->checkLocalState("folder2",
                           make_shared<patch::LocalStateCallback>(
                               [](const std::string &folderPath,
                                  const patch::LocalStateResult &result) {}));
    model->preparePatch("folder1",
                        make_shared<patch::PreparePatchCallback>(
                            [](const std::string &folderPath,
                               const patch::PreparePatchResult &result) {
                              throw std::runtime_error("Test error");
                            }));
    model->preparePatch("folder2",
                        make_shared<patch::PreparePatchCallback>(
                            [](const std::string &folderPath,
                               const patch::PreparePatchResult &result) {}));
    uv_run(&loop, UV_RUN_DEFAULT);
  }
  uv_loop_close(&loop);
}
