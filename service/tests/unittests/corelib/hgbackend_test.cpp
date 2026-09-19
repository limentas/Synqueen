#include "corelib/src/const.hpp"
#include "corelib/src/patch/hgbackend.hpp"
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

namespace fs = std::filesystem;

void removeFileOrFolder(const std::string &folderPath) {
  std::error_code ec;
  fs::remove_all(folderPath, ec);
  if (ec) {
    SPDLOG_ERROR("Failed to remove temporary repo folder: {}. Error: {}",
                 folderPath, ec.message());
  }
}

TEST(HgBackendTest, CtorDtor) {
  // Note: This test may leave leftover files in the data folder
  StandardPaths::initialize("Synqueen-test");
  uv_loop_t loop;
  EXPECT_EQ(uv_loop_init(&loop), 0);
  HgBackend backend(&loop);
  uv_run(&loop, UV_RUN_DEFAULT);
  EXPECT_EQ(uv_loop_close(&loop), 0);

  // Cleanup the data folder after the test
  removeFileOrFolder(StandardPaths::getDataPath().string());
}

// Requires Mercurial to be installed and available in PATH
TEST(HgBackendTest, CheckLocalState) {
  // Note: This test may leave leftover files in the data folder
  StandardPaths::initialize("Synqueen-test");
  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);
  auto backend = new HgBackend(loop.get());

  corral::run(*loop, [&backend, &loop]() -> corral::Task<void> {
    // 1. Create temporary folders to write hg process output to and for the
    // repository itself
    auto tempOutPath =
        co_await createTemporaryFolder("temp_out_XXXXXX", loop.get());
    auto tempRepoPath =
        co_await createTemporaryFolder("temp_repo_XXXXXX", loop.get());

    auto result = co_await backend->checkLocalState(tempRepoPath);
    EXPECT_FALSE(result.initialized);
    EXPECT_FALSE(result.hasUncommittedChanges);

    // 2. Now let's initialize a Mercurial repo in the folder and check
    // the local state again
    auto initResult = std::system(("hg init \"" + tempRepoPath + "\"").c_str());
    EXPECT_EQ(initResult, 0) << "Failed to init repo";

    result = co_await backend->checkLocalState(tempRepoPath);
    EXPECT_TRUE(result.initialized);
    EXPECT_FALSE(result.hasUncommittedChanges);

    // 3. Now let's create a file in the repo and check the local state again
    auto testFilePath = fs::path(tempRepoPath) / "test.txt";
    std::ofstream testFile(testFilePath);
    testFile << "Hello, world!" << std::endl;
    testFile.close();
    result = co_await backend->checkLocalState(tempRepoPath);
    EXPECT_TRUE(result.initialized);
    EXPECT_TRUE(result.hasUncommittedChanges);

    // 4. Now let's stage the file and check the local state again
    auto addResult = std::system(("hg add \"" + testFilePath.string() +
                                  "\" --repository \"" + tempRepoPath + "\"")
                                     .c_str());
    EXPECT_EQ(addResult, 0) << "Failed to add file to repo";
    result = co_await backend->checkLocalState(tempRepoPath);
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
    EXPECT_TRUE(result.initialized);
    EXPECT_TRUE(result.hasUncommittedChanges);
    EXPECT_TRUE(result.hasConflicts);

    removeFileOrFolder(tempRepoPath);
    removeFileOrFolder(tempOutPath);

    co_await backend->shutdown();
    delete backend;
  }); // End of corral::run

  // Cleanup the data folder after the test
  removeFileOrFolder(StandardPaths::getDataPath().string());
}

TEST(HgBackendTest, InitEmptyRepoFolder) {
  // Note: This test may leave leftover files in the data folder
  StandardPaths::initialize("Synqueen-test");
  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);
  auto backend = new HgBackend(loop.get());

  corral::run(*loop, [&backend, &loop]() -> corral::Task<void> {
    auto tempRepoPath =
        co_await createTemporaryFolder("temp_repo_XXXXXX", loop.get());
    auto initResult = co_await backend->initRepoFolder(tempRepoPath);
    EXPECT_THAT(initResult.lastCommitHash, testing::IsEmpty());

    auto result = co_await backend->checkLocalState(tempRepoPath);
    EXPECT_TRUE(result.initialized);
    EXPECT_FALSE(result.hasUncommittedChanges);
    EXPECT_FALSE(result.hasConflicts);
    EXPECT_THAT(result.lastCommitHash, testing::IsEmpty());

    removeFileOrFolder(tempRepoPath);

    co_await backend->shutdown();
    delete backend;
  }); // End of corral::run

  // Cleanup the data folder after the test
  removeFileOrFolder(StandardPaths::getDataPath().string());
}

TEST(HgBackendTest, InitExistentRepoFolder) {
  // Note: This test may leave leftover files in the data folder
  StandardPaths::initialize("Synqueen-test");
  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);
  auto backend = new HgBackend(loop.get());

  corral::run(*loop, [&backend, &loop]() -> corral::Task<void> {
    auto tempRepoPath =
        co_await createTemporaryFolder("temp_repo_XXXXXX", loop.get());

    // Create a file inside the temporary repo folder to simulate an existing
    // repo
    auto testFilePath = tempRepoPath + "/testfile.txt";
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

    removeFileOrFolder(tempRepoPath);

    co_await backend->shutdown();
    delete backend;
  }); // End of corral::run

  // Cleanup the data folder after the test
  removeFileOrFolder(StandardPaths::getDataPath().string());
}

TEST(HgBackendTest, InitRepoFolderErrors) {
  // Note: This test may leave leftover files in the data folder
  StandardPaths::initialize("Synqueen-test");
  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);
  auto backend = new HgBackend(loop.get());

  corral::run(*loop, [&backend, &loop]() -> corral::Task<void> {
    // Create a temporary folder for a testing repo
    auto tempDir = fs::temp_directory_path();
    auto tempPath =
        co_await createTemporaryFolder("temp_repo_XXXXXX", loop.get());
    auto tempRepoPath = (tempDir / "non_existent_folder").string();

    // Folder doesn't exist
    EXPECT_THROW(
        { co_await backend->initRepoFolder(tempRepoPath); },
        synqueen::SqDoesNotExist);

    // The path is not a directory
    auto tempFilePath = (tempDir / "temp_file").string();
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

    removeFileOrFolder(tempPath);
    co_await backend->shutdown();
    delete backend;
  }); // End of corral::run

  // Cleanup the data folder after the test
  removeFileOrFolder(StandardPaths::getDataPath().string());
}

TEST(HgBackendTest, PreparePatch) {
  StandardPaths::initialize("Synqueen-test");

  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);
  auto backend = new HgBackend(loop.get());

  corral::run(*loop, [&backend, &loop]() -> corral::Task<void> {
    // Create a temporary folder for a testing repo
    auto tempDir = fs::temp_directory_path();
    auto tempPath =
        co_await createTemporaryFolder("temp_repo_XXXXXX", loop.get());

    auto initResult = co_await backend->initRepoFolder(tempPath);
    EXPECT_THAT(initResult.lastCommitHash, testing::IsEmpty());

    // Add some files to the repository
    auto testFilePath = fs::path(tempPath) / "test_file.txt";
    std::ofstream(testFilePath).put('a');

    // Prepare a patch from the current state
    auto patchResult = co_await backend->preparePatch(tempPath, "");
    EXPECT_THAT(patchResult.patchFilePath, testing::Not(testing::IsEmpty()));
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
    auto testFile2Path = fs::path(tempPath) / "test_file2.txt";
    std::ofstream(testFile2Path) << "test\n";

    // Prepare a patch from the current state again
    auto patchResult2 = co_await backend->preparePatch(
        tempPath, patchResult.lastIncludedCommitHash);
    EXPECT_THAT(patchResult2.patchFilePath, testing::Not(testing::IsEmpty()));
    EXPECT_THAT(patchResult2.lastIncludedCommitHash,
                testing::Not(testing::IsEmpty()));
    removeFileOrFolder(patchResult2.patchFilePath);

    // Check state again
    localState = co_await backend->checkLocalState(tempPath);
    EXPECT_TRUE(localState.initialized);
    EXPECT_FALSE(localState.hasUncommittedChanges);
    EXPECT_FALSE(localState.hasConflicts);
    EXPECT_EQ(patchResult2.lastIncludedCommitHash, localState.lastCommitHash);

    removeFileOrFolder(tempPath);

    co_await backend->shutdown();
    delete backend;
  }); // End of corral::run

  // Cleanup the data folder after the test
  removeFileOrFolder(StandardPaths::getDataPath().string());
}

TEST(HgBackendTest, ApplyPatches) {
  StandardPaths::initialize("Synqueen-test");

  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);
  auto backend = new HgBackend(loop.get());

  corral::run(*loop, [&backend, &loop]() -> corral::Task<void> {
    // Create temporary folders for 2 testing repos
    auto tempDir = fs::temp_directory_path();
    auto srcRepoPath =
        co_await createTemporaryFolder("temp_repo1_XXXXXX", loop.get());
    auto repo1Path =
        co_await createTemporaryFolder("temp_repo2_XXXXXX", loop.get());

    auto initResult = co_await backend->initRepoFolder(srcRepoPath);
    initResult = co_await backend->initRepoFolder(repo1Path);

    // Add some files to the first repository
    auto testFilePath = fs::path(srcRepoPath) / "test_file.txt";
    std::ofstream(testFilePath) << "test\n";

    // Prepare a patch from the current state
    auto patchResult = co_await backend->preparePatch(srcRepoPath, "");
    EXPECT_THAT(patchResult.patchFilePath, testing::Not(testing::IsEmpty()));
    EXPECT_THAT(patchResult.lastIncludedCommitHash,
                testing::Not(testing::IsEmpty()));

    // Apply the patch to the second repository
    auto applyResult =
        co_await backend->applyPatches(repo1Path, {patchResult.patchFilePath});
    EXPECT_FALSE(applyResult.hasConflicts);
    EXPECT_EQ(applyResult.lastIncludedCommitHash,
              patchResult.lastIncludedCommitHash);

    // Add more changes to the second repository
    std::ofstream(fs::path(repo1Path) / "test_file.txt", std::ios::app)
        << "\nbcde\n";
    auto testFile2Path = fs::path(repo1Path) / "test_file2.txt";
    std::ofstream(testFile2Path) << "test\n";

    removeFileOrFolder(patchResult.patchFilePath);

    // Prepare a patch from the second repo
    patchResult = co_await backend->preparePatch(
        repo1Path, patchResult.lastIncludedCommitHash);
    EXPECT_THAT(patchResult.patchFilePath, testing::Not(testing::IsEmpty()));
    EXPECT_THAT(patchResult.lastIncludedCommitHash,
                testing::Not(testing::IsEmpty()));

    // Apply the second patch to the first repository
    applyResult = co_await backend->applyPatches(srcRepoPath,
                                                 {patchResult.patchFilePath});
    EXPECT_FALSE(applyResult.hasConflicts);
    EXPECT_EQ(applyResult.lastIncludedCommitHash,
              patchResult.lastIncludedCommitHash);
    removeFileOrFolder(patchResult.patchFilePath);

    removeFileOrFolder(repo1Path);
    removeFileOrFolder(srcRepoPath);
    co_await backend->shutdown();
    delete backend;
  }); // End of corral::run

  // Cleanup the data folder after the test
  removeFileOrFolder(StandardPaths::getDataPath().string());
}

// Check that the order of patches being applied doesn't matter
TEST(HgBackendTest, ApplyPatchesOrder) {
  StandardPaths::initialize("Synqueen-test");

  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);
  auto backend = new HgBackend(loop.get());

  corral::run(*loop, [&backend, &loop]() -> corral::Task<void> {
    // Create temporary folders for 3 testing repos
    auto tempDir = fs::temp_directory_path();
    auto srcRepoPath =
        co_await createTemporaryFolder("temp_src_repo_XXXXXX", loop.get());
    auto repo1Path =
        co_await createTemporaryFolder("temp_repo1_XXXXXX", loop.get());
    auto repo2Path =
        co_await createTemporaryFolder("temp_repo2_XXXXXX", loop.get());

    auto initResult = co_await backend->initRepoFolder(srcRepoPath);
    initResult = co_await backend->initRepoFolder(repo1Path);
    initResult = co_await backend->initRepoFolder(repo2Path);

    std::list<fs::path> patchFiles;
    std::string commitHashes;
    // Add a file to the first repository
    auto testFilePath = fs::path(srcRepoPath) / "test_file.txt";
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
        {"log", "--template", "{node}\\n", "--repository", repo1Path});
    EXPECT_EQ(commitHashes, logResult.output);

    // Now let's apply the patches in reverse order
    applyResult = co_await backend->applyPatches(repo2Path, patchFiles);
    EXPECT_FALSE(applyResult.hasConflicts);
    EXPECT_EQ(applyResult.lastIncludedCommitHash,
              patchResult.lastIncludedCommitHash);
    logResult = co_await hg.runCommand(
        {"log", "--template", "{node}\\n", "--repository", repo2Path});
    EXPECT_EQ(commitHashes, logResult.output);

    for (const auto &patchFile : patchFiles) {
      removeFileOrFolder(patchFile.string());
    }
    removeFileOrFolder(srcRepoPath);
    removeFileOrFolder(repo1Path);
    removeFileOrFolder(repo2Path);
    co_await hg.shutdown();
    co_await backend->shutdown();
    delete backend;
  }); // End of corral::run

  // Cleanup the data folder after the test
  removeFileOrFolder(StandardPaths::getDataPath().string());
}
