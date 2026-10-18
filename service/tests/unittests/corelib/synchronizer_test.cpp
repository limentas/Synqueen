#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "synchronizer.hpp"
#include "testutils.hpp"
#include "utils/corraleventlooptraits.hpp"
#include "utils/corralheader.hpp"
#include "utils/standardpaths.hpp"

using namespace testing;
using namespace synqueen;
using namespace synqueen::utils;

namespace fs = std::filesystem;

class SynchronizerTest : public testing::Test {
protected:
  void SetUp() override { StandardPaths::initializeInTemp("Synqueen-test"); }
  void TearDown() override { cleanupAppFolders(); }
};

TEST_F(SynchronizerTest, CtorDtor) {
  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);
  corral::run(*loop, [&loop]() -> corral::Task<void> {
    auto tempFolder = createTemporaryFolder("sq_test");
    Settings settings;
    Synchronizer synchronizer(*loop, settings, [](const Settings &s) {});
    co_return;
  });
}
