#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "corelib/src/patch/hgbackend.hpp"
#include "utils/uvutils.hpp"

using namespace testing;
using namespace std;
using namespace std::string_literals;
using namespace synqueen;

TEST(HgBackendTest, CtorDtor) {
  uv_loop_t loop;
  EXPECT_EQ(uv_loop_init(&loop), 0);
  HgBackend backend(&loop);
  uv_run(&loop, UV_RUN_DEFAULT);
  EXPECT_EQ(uv_loop_close(&loop), 0);
}

TEST(HgBackendTest, CheckLocalState) {
  auto l = new uv_loop_t();
  auto result = uv_loop_init(l);
  EXPECT_EQ(result, 0);
  auto loop = LoopPtr(l, deleteLoop);
  auto backend = new HgBackend(loop.get());
  bool callbackCalled = false;

  auto callback = make_shared<patch::LocalStateCallback>(
      [backend, &callbackCalled](const std::string &folderPath,
                                 const patch::LocalStateResult &result) {
        callbackCalled = true;
        EXPECT_TRUE(result.ok);
        EXPECT_FALSE(result.initialized);
        EXPECT_FALSE(result.hasUncommittedChanges);
        delete backend;
      });
  backend->checkLocalState("non-existent-folder", callback);

  uv_run(loop.get(), UV_RUN_DEFAULT);
  EXPECT_TRUE(callbackCalled);
}
