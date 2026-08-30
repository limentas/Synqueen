#include "corelib/src/patch/hgbackend.hpp"
#include "utils/corraleventlooptraits.hpp"
#include "utils/corralheader.hpp"
#include "utils/uvutils.hpp"

#include <cstdlib>
#include <filesystem>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

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

TEST(HgBackendTest, CheckLocalStateNonExistentFolder) {
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

TEST(HgBackendTest, CheckLocalStateExistingFolder) {
// TODO: create a temporary folder with a Mercurial repo and test
// checkLocalState on it
#if 0
  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);
  auto backend = new HgBackend(loop.get());

  // Create a temporary folder and initialize a Mercurial repo in it
  auto tmpDir = std::filesystem::temp_directory_path();
  auto tempRepoTemplate = tmpDir.string() + "/temp_repo_XXXXXX";
  uv_fs_t req;
  using CBPType = corral::CBPortal<uv_fs_t *>;
  CBPType cbp;
  corral::run(
      *loop, [&loop, &tempRepoTemplate, &req, &cbp]() -> corral::Task<void> {
        auto r = co_await corral::untilCBCalled(
            [&](CBPType::Callback &cb) {
              req.data = &cb;
              uv_fs_mkdtemp(
                  loop.get(), &req, tempRepoTemplate.c_str(),
                  +[](uv_fs_t *r) { (*(CBPType::Callback *)r->data)(r); });
            },
            cbp);
        // Otherwise ASSERT_* will not work inside a coroutine.
        [&r]() {
          ASSERT_GE(r->result, 0)
              << "Failed to create temporary folder for repo";
        }();

        auto tempRepoPath = std::string(r->path);
        uv_fs_req_cleanup(&req);
        SPDLOG_INFO("Created temporary repo folder: {}", tempRepoPath);

        // Now init repo
        std::system(("hg init " + tempRepoPath).c_str());

        auto result = co_await backend->checkLocalState(tempRepoPath);
        co_await backend->shutdown();
        delete backend;
        EXPECT_TRUE(result.initialized);
      });
#endif
}
