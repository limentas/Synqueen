#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>

#include "corelib/src/patchexchange.hpp"

using namespace testing;
using namespace std;
using namespace synqueen;

TEST(PatchExchangeTest, CtorDtor) {
  auto loop = uv_default_loop();
  {
    auto model = make_unique<PatchExchange>(loop);
    uv_run(loop, UV_RUN_DEFAULT);
  }
  uv_loop_close(loop);
}

// Test overall state logic
struct HgOutputTestParameters {
  std::string name;
  std::string input;

  PatchExchange::HgOutput expected_output;
};

inline void PrintTo(const HgOutputTestParameters &p, ::std::ostream *os) {
  *os << "{ name=" << p.name << " input=" << p.input << " }";
}

class HgOutputTest : public ::testing::TestWithParam<HgOutputTestParameters> {};

TEST_P(HgOutputTest, ParseHgOutput) {
  const auto &param = GetParam();
  auto loop = uv_default_loop();
  {
    auto model = make_unique<PatchExchange>(loop);
    PatchExchange::HgOutput actualOutput;
    model->parseHgOutput(param.input.c_str(), param.input.size(), actualOutput);
    EXPECT_EQ(actualOutput.channel, param.expected_output.channel);
    EXPECT_EQ(actualOutput.data, param.expected_output.data);
  }
  uv_loop_close(loop);
}

INSTANTIATE_TEST_SUITE_P(
    HgOutputTestInstance, HgOutputTest,
    testing::Values(
        // All good
        HgOutputTestParameters{
            .name = "All good",
            .input = "some input",
            .expected_output =
                PatchExchange::HgOutput{PatchExchange::HgChannel::Output,
                                        "some output"}},
        HgOutputTestParameters{
            .name = "One Active",
            .input = "some input",
            .expected_output = PatchExchange::HgOutput{
                PatchExchange::HgChannel::Output, "some output"}}));
