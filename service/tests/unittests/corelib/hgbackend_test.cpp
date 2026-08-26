#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "corelib/src/patch/hgbackend.hpp"
#include "utils/corraleventlooptraits.hpp"
#include "utils/corralheader.hpp"
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
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);
  auto backend = new HgBackend(loop.get());

  auto result =
      corral::run(*loop, [&backend]() -> corral::Task<patch::LocalStateResult> {
        auto t = backend->checkLocalState("non-existent-folder");
        auto result = co_await t;
        co_await backend->shutdown();
        delete backend;
        co_return result;
      });
  EXPECT_TRUE(result.ok);
  EXPECT_FALSE(result.initialized);
  EXPECT_FALSE(result.hasUncommittedChanges);
}
