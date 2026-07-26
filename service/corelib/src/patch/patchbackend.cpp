#include "patchbackend.hpp"

#include "hgbackend.hpp"

namespace synqueen {

PatchBackend *synqueen::createPatchBackend(uv_loop_t *loop) {
  return new HgBackend(loop);
}

} // namespace synqueen
