#include "patchbackend.hpp"

#include "hgbackend.hpp"
#include "utils/exceptions.hpp"

namespace fs = std::filesystem;

namespace synqueen::patch {

PatchBackend *createPatchBackend(uv_loop_t &l, corral::Nursery &n) {
  return new HgBackend(l, n);
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
  return initRepoFolderImpl(folderPath);
}

} // namespace synqueen::patch
