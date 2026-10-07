#include "patchbackend.hpp"

#include "hgbackend.hpp"
#include "utils/exceptions.hpp"

namespace fs = std::filesystem;

namespace synqueen::patch {

PatchBackend *createPatchBackend(uv_loop_t *loop) {
  return new HgBackend(loop);
}

corral::Task<InitRepoResult>
PatchBackend::initRepoFolder(const std::filesystem::path &folderPath) {
  if (!fs::exists(folderPath)) {
    throw synqueen::SqDoesNotExist("The specified folder does not exist: " +
                                   folderPath.string());
  }
  if (!fs::is_directory(folderPath)) {
    throw synqueen::SqNotADirectory("The specified path is not a directory: " +
                                    folderPath.string());
  }
  co_return co_await initRepoFolderImpl(folderPath);
}

} // namespace synqueen::patch
