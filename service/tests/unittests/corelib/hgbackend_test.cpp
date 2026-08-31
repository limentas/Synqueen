#include "corelib/src/patch/hgbackend.hpp"
#include "utils/corraleventlooptraits.hpp"
#include "utils/corralheader.hpp"
#include "utils/uvutils.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
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

// Requires Mercurial to be installed and available in PATH
TEST(HgBackendTest, CheckLocalState) {
  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);
  auto backend = new HgBackend(loop.get());

  corral::run(*loop, [&backend, &loop]() -> corral::Task<void> {
    // 1. Create a temporary folder to write hg process output to
    auto tmpDir = std::filesystem::temp_directory_path();
    auto tempOutTemplate = tmpDir.string() + "/temp_out_XXXXXX";
    uv_fs_t req;
    using CBPType = corral::CBPortal<uv_fs_t *>;
    CBPType cbp;
    auto r = co_await corral::untilCBCalled(
        [&](CBPType::Callback &cb) {
          req.data = &cb;
          uv_fs_mkdtemp(
              loop.get(), &req, tempOutTemplate.c_str(),
              +[](uv_fs_t *r) { (*(CBPType::Callback *)r->data)(r); });
        },
        cbp);
    // Otherwise ASSERT_* will not work inside a coroutine.
    [&r]() {
      ASSERT_GE(r->result, 0)
          << "Failed to create temporary folder for hg process output";
    }();
    auto tempOutPath = std::string(r->path);
    uv_fs_req_cleanup(&req);

    // 2. Create a temporary folder for a testing repo and check the local state
    // on it
    auto tempRepoTemplate = tmpDir.string() + "/temp_repo_XXXXXX";
    r = co_await corral::untilCBCalled(
        [&](CBPType::Callback &cb) {
          req.data = &cb;
          uv_fs_mkdtemp(
              loop.get(), &req, tempRepoTemplate.c_str(),
              +[](uv_fs_t *r) { (*(CBPType::Callback *)r->data)(r); });
        },
        cbp);
    // Otherwise ASSERT_* will not work inside a coroutine.
    [&r]() {
      ASSERT_GE(r->result, 0) << "Failed to create temporary folder for repo";
    }();
    auto tempRepoPath = std::string(r->path);
    uv_fs_req_cleanup(&req);
    SPDLOG_INFO("Created temporary repo folder: {}", tempRepoPath);

    auto result = co_await backend->checkLocalState(tempRepoPath);
    EXPECT_TRUE(result.ok);
    EXPECT_FALSE(result.initialized);
    EXPECT_FALSE(result.hasUncommittedChanges);

    // 2. Now let's initialize a Mercurial repo in the folder and check
    // the local state again
    auto initResult = std::system(("hg init \"" + tempRepoPath + "\"").c_str());
    EXPECT_EQ(initResult, 0) << "Failed to init repo";

    result = co_await backend->checkLocalState(tempRepoPath);
    EXPECT_TRUE(result.ok);
    EXPECT_TRUE(result.initialized);
    EXPECT_FALSE(result.hasUncommittedChanges);

    // 3. Now let's create a file in the repo and check the local state again
    auto testFilePath = std::filesystem::path(tempRepoPath) / "test.txt";
    std::ofstream testFile(testFilePath);
    testFile << "Hello, world!" << std::endl;
    testFile.close();
    result = co_await backend->checkLocalState(tempRepoPath);
    EXPECT_TRUE(result.ok);
    EXPECT_TRUE(result.initialized);
    EXPECT_TRUE(result.hasUncommittedChanges);

    // 4. Now let's stage the file and check the local state again
    auto addResult = std::system(("hg add \"" + testFilePath.string() +
                                  "\" --repository \"" + tempRepoPath + "\"")
                                     .c_str());
    EXPECT_EQ(addResult, 0) << "Failed to add file to repo";
    result = co_await backend->checkLocalState(tempRepoPath);
    EXPECT_TRUE(result.ok);
    EXPECT_TRUE(result.initialized);
    EXPECT_TRUE(result.hasUncommittedChanges);

    // 5. Now let's commit the file and check the local state again
    auto commitResult = std::system(
        std::string("hg commit -m \"Initial commit\" --user "
                    "\"Test User <test@example.com>\" --repository \"" +
                    tempRepoPath + "\"")
            .c_str());
    EXPECT_EQ(commitResult, 0) << "Failed to commit file to repo";
    result = co_await backend->checkLocalState(tempRepoPath);
    EXPECT_TRUE(result.ok);
    EXPECT_TRUE(result.initialized);
    EXPECT_FALSE(result.hasUncommittedChanges);

    // 5a. Now let's read last commit hash and check it
    auto idResult = std::system(
        std::string("hg id -i --debug --repository \"" + tempRepoPath +
                    "\" > \"" + tempOutPath + "/last_commit_hash.txt\"")
            .c_str());
    EXPECT_EQ(idResult, 0) << "Failed to get last commit hash";
    std::ifstream lastCommitFile(tempOutPath + "/last_commit_hash.txt");
    std::string lastCommitHash;
    std::getline(lastCommitFile, lastCommitHash);
    EXPECT_FALSE(lastCommitHash.empty()) << "Last commit hash is empty";
    EXPECT_EQ(result.lastCommitHash, lastCommitHash)
        << "Last commit hash does not match";

    // 6. Now let's create a conflict by committing a change to this file
    // from a new branch and then committing a change to the same file from
    // default branch
    auto branchResult =
        std::system(std::string("hg branch new_branch --repository \"" +
                                tempRepoPath + "\"")
                        .c_str());
    EXPECT_EQ(branchResult, 0) << "Failed to create new branch";

    // 6a. Modifying the same file
    std::ofstream testFile2(testFilePath, ios_base::out | ios_base::trunc);
    testFile2 << "Hello, another world!" << std::endl;
    testFile2.close();

    // 6b. Commit the change to the new branch
    commitResult = std::system(
        std::string("hg commit -m \"Commit on new branch\" --user "
                    "\"Test User <test@example.com>\" --repository \"" +
                    tempRepoPath + "\"")
            .c_str());
    EXPECT_EQ(commitResult, 0) << "Failed to commit on new branch";

    // 6c. Now let's switch back to the default branch and commit a change to
    // the same file
    auto updateResult = std::system(
        std::string("hg update default --repository \"" + tempRepoPath + "\"")
            .c_str());
    EXPECT_EQ(updateResult, 0) << "Failed to update to default branch";

    // 6d. Modifying the same file again
    std::ofstream testFile3(testFilePath, ios_base::out | ios_base::trunc);
    testFile3 << "Hello, yet another world!" << std::endl;
    testFile3.close();

    // 6e. Commit the change to the default branch
    commitResult = std::system(
        std::string("hg commit -m \"Commit on default branch\" --user "
                    "\"Test User <test@example.com>\" --repository \"" +
                    tempRepoPath + "\"")
            .c_str());
    EXPECT_EQ(commitResult, 0) << "Failed to commit on default branch";

    // 6f. Now let's try to merge the new branch into the default branch and
    // check the local state again
    auto mergeResult = std::system(
        std::string(
            "hg merge new_branch --tool internal:merge --repository \"" +
            tempRepoPath + "\"")
            .c_str());

    // 6g. Now let's check the local state again
    result = co_await backend->checkLocalState(tempRepoPath);
    EXPECT_TRUE(result.ok);
    EXPECT_TRUE(result.initialized);
    EXPECT_TRUE(result.hasUncommittedChanges);
    EXPECT_TRUE(result.hasConflicts);

    // Last. Cleanup temporary folder
    std::error_code ec;
    std::filesystem::remove_all(tempRepoPath, ec);
    if (ec) {
      SPDLOG_ERROR("Failed to remove temporary repo folder: {}. Error: {}",
                   tempRepoPath, ec.message());
    }

    co_await backend->shutdown();
    delete backend;
  });
}
