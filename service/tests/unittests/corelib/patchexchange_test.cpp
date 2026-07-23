#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>

#include "corelib/src/patch/patchexchange.hpp"

using namespace testing;
using namespace std;
using namespace synqueen;

TEST(PatchExchangeTest, CtorDtor) {
  uv_loop_t loop;
  uv_loop_init(&loop);
  {
    auto model = make_unique<PatchExchange>(&loop);
    uv_run(&loop, UV_RUN_DEFAULT);
  }
  uv_loop_close(&loop);
}
