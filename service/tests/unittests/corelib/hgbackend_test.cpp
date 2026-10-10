#include "corelib/src/const.hpp"
#include "corelib/src/patch/hgbackend.hpp"
#include "testutils.hpp"
#include "utils/corraleventlooptraits.hpp"
#include "utils/corralheader.hpp"
#include "utils/exceptions.hpp"
#include "utils/standardpaths.hpp"
#include "utils/utils.hpp"
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
using namespace synqueen::patch;
using namespace synqueen::utils;

namespace fs = std::filesystem;

class HgBackendTest : public testing::Test {
protected:
  void SetUp() override { StandardPaths::initializeInTemp("Synqueen-test"); }
  void TearDown() override { cleanupAppFolders(); }
};

TEST_F(HgBackendTest, CtorDtor) {
  uv_loop_t loop;
  EXPECT_EQ(uv_loop_init(&loop), 0);
  HgBackend backend(&loop);
  uv_run(&loop, UV_RUN_DEFAULT);
  EXPECT_EQ(uv_loop_close(&loop), 0);
}

// Requires Mercurial to be installed and available in PATH
TEST_F(HgBackendTest, CheckLocalState) {
  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);
  auto backend = new HgBackend(loop.get());

  corral::run(*loop, [&backend, &loop]() -> corral::Task<void> {
    // 1. Create temporary folders to write hg process output to and for the
    // repository itself
    auto tempOutPath = createTemporaryFolder("temp_out");
    auto tempRepoPath = createTemporaryFolder("temp_repo");

    auto result = co_await backend->checkLocalState(tempRepoPath);
    EXPECT_FALSE(result.initialized);
    EXPECT_FALSE(result.hasUncommittedChanges);

    // 2. Now let's initialize a Mercurial repo in the folder and check
    // the local state again
    auto initResult =
        std::system(("hg init \"" + tempRepoPath.string() + "\"").c_str());
    EXPECT_EQ(initResult, 0) << "Failed to init repo";

    result = co_await backend->checkLocalState(tempRepoPath);
    EXPECT_TRUE(result.initialized);
    EXPECT_FALSE(result.hasUncommittedChanges);

    // 3. Now let's create a file in the repo and check the local state again
    auto testFilePath = tempRepoPath / "test.txt";
    std::ofstream testFile(testFilePath);
    testFile << "Hello, world!" << std::endl;
    testFile.close();
    result = co_await backend->checkLocalState(tempRepoPath);
    EXPECT_TRUE(result.initialized);
    EXPECT_TRUE(result.hasUncommittedChanges);

    // 4. Now let's stage the file and check the local state again
    auto addResult =
        std::system(("hg add \"" + testFilePath.string() +
                     "\" --repository \"" + tempRepoPath.string() + "\"")
                        .c_str());
    EXPECT_EQ(addResult, 0) << "Failed to add file to repo";
    result = co_await backend->checkLocalState(tempRepoPath);
    EXPECT_TRUE(result.initialized);
    EXPECT_TRUE(result.hasUncommittedChanges);

    // 5. Now let's commit the file and check the local state again
    auto commitResult = std::system(
        std::string("hg commit -m \"Initial commit\" --user "
                    "\"Test User <test@example.com>\" --repository \"" +
                    tempRepoPath.string() + "\"")
            .c_str());
    EXPECT_EQ(commitResult, 0) << "Failed to commit file to repo";
    result = co_await backend->checkLocalState(tempRepoPath);
    EXPECT_TRUE(result.initialized);
    EXPECT_FALSE(result.hasUncommittedChanges);

    // 5a. Now let's read last commit hash and check it
    auto idResult = std::system(std::string("hg id -i --debug --repository \"" +
                                            tempRepoPath.string() + "\" > \"" +
                                            tempOutPath.string() +
                                            "/last_commit_hash.txt\"")
                                    .c_str());
    EXPECT_EQ(idResult, 0) << "Failed to get last commit hash";
    std::ifstream lastCommitFile(tempOutPath.string() +
                                 "/last_commit_hash.txt");
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
                                tempRepoPath.string() + "\"")
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
                    tempRepoPath.string() + "\"")
            .c_str());
    EXPECT_EQ(commitResult, 0) << "Failed to commit on new branch";

    // 6c. Now let's switch back to the default branch and commit a change to
    // the same file
    auto updateResult =
        std::system(std::string("hg update default --repository \"" +
                                tempRepoPath.string() + "\"")
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
                    tempRepoPath.string() + "\"")
            .c_str());
    EXPECT_EQ(commitResult, 0) << "Failed to commit on default branch";

    // 6f. Now let's try to merge the new branch into the default branch and
    // check the local state again
    auto mergeResult = std::system(
        std::string(
            "hg merge new_branch --tool internal:merge --repository \"" +
            tempRepoPath.string() + "\"")
            .c_str());

    // 6g. Now let's check the local state again
    result = co_await backend->checkLocalState(tempRepoPath);
    EXPECT_TRUE(result.initialized);
    EXPECT_TRUE(result.hasUncommittedChanges);
    EXPECT_TRUE(result.hasConflicts);

    co_await backend->shutdown();
    delete backend;
  }); // End of corral::run
}

TEST_F(HgBackendTest, InitEmptyRepoFolder) {
  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);
  auto backend = new HgBackend(loop.get());

  corral::run(*loop, [&backend, &loop]() -> corral::Task<void> {
    auto tempRepoPath = createTemporaryFolder("temp_repo");
    auto initResult = co_await backend->initRepoFolder(tempRepoPath);
    EXPECT_THAT(initResult.lastCommitHash, testing::IsEmpty());

    auto result = co_await backend->checkLocalState(tempRepoPath);
    EXPECT_TRUE(result.initialized);
    EXPECT_FALSE(result.hasUncommittedChanges);
    EXPECT_FALSE(result.hasConflicts);
    EXPECT_THAT(result.lastCommitHash, testing::IsEmpty());

    co_await backend->shutdown();
    delete backend;
  }); // End of corral::run
}

TEST_F(HgBackendTest, InitExistentRepoFolder) {
  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);
  auto backend = new HgBackend(loop.get());

  corral::run(*loop, [&backend, &loop]() -> corral::Task<void> {
    auto tempRepoPath = createTemporaryFolder("temp_repo");

    // Create a file inside the temporary repo folder to simulate an existing
    // repo
    auto testFilePath = tempRepoPath / "testfile.txt";
    std::ofstream testFile(testFilePath);
    testFile << "This is a test file." << std::endl;
    testFile.close();

    auto initResult = co_await backend->initRepoFolder(tempRepoPath);
    EXPECT_THAT(initResult.lastCommitHash, testing::Not(testing::IsEmpty()));

    auto result = co_await backend->checkLocalState(tempRepoPath);
    EXPECT_TRUE(result.initialized);
    EXPECT_FALSE(result.hasUncommittedChanges);
    EXPECT_FALSE(result.hasConflicts);
    EXPECT_THAT(result.lastCommitHash, testing::Not(testing::IsEmpty()));

    co_await backend->shutdown();
    delete backend;
  }); // End of corral::run
}

TEST_F(HgBackendTest, InitRepoFolderErrors) {
  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);
  auto backend = new HgBackend(loop.get());

  corral::run(*loop, [&backend, &loop]() -> corral::Task<void> {
    // Create a temporary folder for a testing repo
    auto tempDir = createTemporaryFolder("temp_dir");
    auto tempPath = createTemporaryFolder("temp_repo");

    // Folder doesn't exist
    EXPECT_THROW(
        { co_await backend->initRepoFolder(tempDir / "non_existent_folder"); },
        synqueen::SqDoesNotExist);

    // The path is not a directory
    auto tempFilePath = (tempDir / "temp_file");
    std::ofstream(tempFilePath).put('a');
    EXPECT_THROW(
        { co_await backend->initRepoFolder(tempFilePath); },
        synqueen::SqNotADirectory);
    removeFileOrFolder(tempFilePath);

    // Initialize the repository that is already used
    co_await backend->initRepoFolder(tempPath);
    EXPECT_THROW(
        { co_await backend->initRepoFolder(tempPath); },
        synqueen::SqAlreadyUsed);

    co_await backend->shutdown();
    delete backend;
  }); // End of corral::run
}

TEST_F(HgBackendTest, PreparePatch) {
  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);
  auto backend = new HgBackend(loop.get());

  corral::run(*loop, [&backend, &loop]() -> corral::Task<void> {
    // Create a temporary folder for a testing repo
    auto tempPath = createTemporaryFolder("temp_repo");

    auto initResult = co_await backend->initRepoFolder(tempPath);
    EXPECT_THAT(initResult.lastCommitHash, testing::IsEmpty());

    // Add some files to the repository
    auto testFilePath = tempPath / "test_file.txt";
    std::ofstream(testFilePath).put('a');

    // Prepare a patch from the current state
    auto patchResult = co_await backend->preparePatch(tempPath, "");
    EXPECT_THAT(patchResult.patchFilePath.string(),
                testing::Not(testing::IsEmpty()));
    EXPECT_THAT(patchResult.lastIncludedCommitHash,
                testing::Not(testing::IsEmpty()));
    removeFileOrFolder(patchResult.patchFilePath);

    // Check state
    auto localState = co_await backend->checkLocalState(tempPath);
    EXPECT_TRUE(localState.initialized);
    EXPECT_FALSE(localState.hasUncommittedChanges);
    EXPECT_FALSE(localState.hasConflicts);
    EXPECT_EQ(patchResult.lastIncludedCommitHash, localState.lastCommitHash);

    // Add more changes to the repository
    std::ofstream(testFilePath, std::ios::app) << "\nbcde\n";
    auto testFile2Path = tempPath / "test_file2.txt";
    std::ofstream(testFile2Path) << "test\n";

    // Prepare a patch from the current state again
    auto patchResult2 = co_await backend->preparePatch(
        tempPath, patchResult.lastIncludedCommitHash);
    EXPECT_THAT(patchResult2.patchFilePath.string(),
                testing::Not(testing::IsEmpty()));
    EXPECT_THAT(patchResult2.lastIncludedCommitHash,
                testing::Not(testing::IsEmpty()));
    removeFileOrFolder(patchResult2.patchFilePath);

    // Check state again
    localState = co_await backend->checkLocalState(tempPath);
    EXPECT_TRUE(localState.initialized);
    EXPECT_FALSE(localState.hasUncommittedChanges);
    EXPECT_FALSE(localState.hasConflicts);
    EXPECT_EQ(patchResult2.lastIncludedCommitHash, localState.lastCommitHash);

    co_await backend->shutdown();
    delete backend;
  }); // End of corral::run
}

TEST_F(HgBackendTest, ApplyPatches) {
  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);
  auto backend = new HgBackend(loop.get());

  corral::run(*loop, [&backend, &loop]() -> corral::Task<void> {
    // Create temporary folders for 2 testing repos
    auto srcRepoPath = createTemporaryFolder("temp_repo1");
    auto repo1Path = createTemporaryFolder("temp_repo2");

    auto initResult = co_await backend->initRepoFolder(srcRepoPath);
    initResult = co_await backend->initRepoFolder(repo1Path);

    // Add some files to the first repository
    auto testFilePath = srcRepoPath / "test_file.txt";
    std::ofstream(testFilePath) << "test\n";

    // Prepare a patch from the current state
    auto patchResult = co_await backend->preparePatch(srcRepoPath, "");
    EXPECT_THAT(patchResult.patchFilePath.string(),
                testing::Not(testing::IsEmpty()));
    EXPECT_THAT(patchResult.lastIncludedCommitHash,
                testing::Not(testing::IsEmpty()));

    // Apply the patch to the second repository
    auto applyResult =
        co_await backend->applyPatches(repo1Path, {patchResult.patchFilePath});
    EXPECT_FALSE(applyResult.hasConflicts);
    EXPECT_EQ(applyResult.lastIncludedCommitHash,
              patchResult.lastIncludedCommitHash);

    // Add more changes to the second repository
    std::ofstream(repo1Path / "test_file.txt", std::ios::app) << "\nbcde\n";
    auto testFile2Path = repo1Path / "test_file2.txt";
    std::ofstream(testFile2Path) << "test\n";

    removeFileOrFolder(patchResult.patchFilePath);

    // Prepare a patch from the second repo
    patchResult = co_await backend->preparePatch(
        repo1Path, patchResult.lastIncludedCommitHash);
    EXPECT_THAT(patchResult.patchFilePath.string(),
                testing::Not(testing::IsEmpty()));
    EXPECT_THAT(patchResult.lastIncludedCommitHash,
                testing::Not(testing::IsEmpty()));

    // Apply the second patch to the first repository
    applyResult = co_await backend->applyPatches(srcRepoPath,
                                                 {patchResult.patchFilePath});
    EXPECT_FALSE(applyResult.hasConflicts);
    EXPECT_EQ(applyResult.lastIncludedCommitHash,
              patchResult.lastIncludedCommitHash);
    removeFileOrFolder(patchResult.patchFilePath);

    co_await backend->shutdown();
    delete backend;
  }); // End of corral::run
}

// Check that the order of patches being applied doesn't matter
TEST_F(HgBackendTest, ApplyPatchesOrder) {
  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);
  auto backend = new HgBackend(loop.get());

  corral::run(*loop, [&backend, &loop]() -> corral::Task<void> {
    // Create temporary folders for 3 testing repos
    auto srcRepoPath = createTemporaryFolder("temp_src_repo");
    auto repo1Path = createTemporaryFolder("temp_repo1");
    auto repo2Path = createTemporaryFolder("temp_repo2");

    auto initResult = co_await backend->initRepoFolder(srcRepoPath);
    initResult = co_await backend->initRepoFolder(repo1Path);
    initResult = co_await backend->initRepoFolder(repo2Path);

    std::list<fs::path> patchFiles;
    std::string commitHashes;
    // Add a file to the first repository
    auto testFilePath = srcRepoPath / "test_file.txt";
    std::ofstream(testFilePath) << "test\n";
    auto patchResult = co_await backend->preparePatch(srcRepoPath, "");
    patchFiles.push_back(patchResult.patchFilePath);
    commitHashes += patchResult.lastIncludedCommitHash + "\n";

    std::ofstream(testFilePath, std::ios_base::trunc | std::ios_base::in)
        << "reset content\n";
    patchResult = co_await backend->preparePatch(
        srcRepoPath, patchResult.lastIncludedCommitHash);
    patchFiles.push_back(patchResult.patchFilePath);
    commitHashes.insert(0, patchResult.lastIncludedCommitHash + "\n");

    std::ofstream(testFilePath) << "line 2\n";
    patchResult = co_await backend->preparePatch(
        srcRepoPath, patchResult.lastIncludedCommitHash);
    patchFiles.push_back(patchResult.patchFilePath);
    commitHashes.insert(0, patchResult.lastIncludedCommitHash + "\n");

    // Apply the patches to another repo in right order
    auto applyResult = co_await backend->applyPatches(repo1Path, patchFiles);
    EXPECT_FALSE(applyResult.hasConflicts);
    EXPECT_EQ(applyResult.lastIncludedCommitHash,
              patchResult.lastIncludedCommitHash);

    HgProcess hg(loop.get());
    auto logResult = co_await hg.runCommand(
        {"log", "--template", "{node}\\n", "--repository", repo1Path.string()});
    EXPECT_EQ(commitHashes, logResult.output);

    // Now let's apply the patches in reverse order
    applyResult = co_await backend->applyPatches(repo2Path, patchFiles);
    EXPECT_FALSE(applyResult.hasConflicts);
    EXPECT_EQ(applyResult.lastIncludedCommitHash,
              patchResult.lastIncludedCommitHash);
    logResult = co_await hg.runCommand(
        {"log", "--template", "{node}\\n", "--repository", repo2Path.string()});
    EXPECT_EQ(commitHashes, logResult.output);

    for (const auto &patchFile : patchFiles) {
      removeFileOrFolder(patchFile);
    }
    co_await hg.shutdown();
    co_await backend->shutdown();
    delete backend;
  }); // End of corral::run
}
