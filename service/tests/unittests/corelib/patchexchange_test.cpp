#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>

#include "corelib/src/patchexchange.hpp"

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

// Test overall state logic
struct HgOutputTestParameters {
  std::string name;
  std::string input;

  PatchExchange::HgOutput expected_output;
};

inline void PrintTo(const HgOutputTestParameters &p, ::std::ostream *os) {
  *os << "{ name=" << p.name << " }";
}

class HgOutputTest : public ::testing::TestWithParam<HgOutputTestParameters> {};

TEST_P(HgOutputTest, ParseHgOutput) {
  const auto &param = GetParam();
  uv_loop_t loop;
  uv_loop_init(&loop);
  {
    auto model = make_unique<PatchExchange>(&loop);
    PatchExchange::HgOutput actualOutput;
    model->parseHgOutput(param.input.c_str(), param.input.size(), actualOutput);
    EXPECT_EQ(actualOutput.channel, param.expected_output.channel);
    EXPECT_EQ(actualOutput.hasData, param.expected_output.hasData);
    if (param.expected_output.hasData) {
      EXPECT_EQ(actualOutput.data, param.expected_output.data);
    }
  }
  uv_loop_close(&loop);
}

INSTANTIATE_TEST_SUITE_P(
    HgOutputTestInstance, HgOutputTest,
    testing::Values(
        HgOutputTestParameters{
            .name = "Docs example",
            .input = std::string("r\x05\x00\x00\x00"
                                 "ascii",
                                 10),
            .expected_output =
                PatchExchange::HgOutput{true, PatchExchange::HgChannel::Result,
                                        "ascii"}},
        HgOutputTestParameters{
            .name = "Another docs example",
            .input = std::string("o\x10\x00\x00\x00"
                                 "branch: default\n",
                                 21),
            .expected_output =
                PatchExchange::HgOutput{true, PatchExchange::HgChannel::Output,
                                        "branch: default\n"}},
        HgOutputTestParameters{
            .name = "Input channel",
            .input = std::string("I\xFF\x00\x00\x00", 5),
            .expected_output =
                PatchExchange::HgOutput{false, PatchExchange::HgChannel::Input,
                                        ""}},
        HgOutputTestParameters{
            .name = "Line channel",
            .input = std::string("L", 1),
            .expected_output = PatchExchange::HgOutput{
                false, PatchExchange::HgChannel::Line, ""}}));
