#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

#include "corelib/src/patch/hgprotocol.hpp"

using namespace testing;
using namespace std;
using namespace synqueen;

// Test overall state logic
struct HgOutputTestParameters {
  std::string name;
  std::string input;

  HgProtocol::HgOutput expected_output;
};

inline void PrintTo(const HgOutputTestParameters &p, ::std::ostream *os) {
  *os << "{ name=" << p.name << " }";
}

class HgOutputTest : public ::testing::TestWithParam<HgOutputTestParameters> {};

TEST_P(HgOutputTest, ParseHgOutput) {
  const auto &param = GetParam();
  HgProtocol::HgOutput actualOutput;
  HgProtocol::parseHgOutput(param.input.c_str(), param.input.size(),
                            actualOutput);
  EXPECT_EQ(actualOutput.channel, param.expected_output.channel);
  EXPECT_EQ(actualOutput.hasData, param.expected_output.hasData);
  if (param.expected_output.hasData) {
    EXPECT_EQ(actualOutput.data, param.expected_output.data);
  }
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
                HgProtocol::HgOutput{true, HgProtocol::HgChannel::Result,
                                     "ascii"}},
        HgOutputTestParameters{
            .name = "Another docs example",
            .input = std::string("o\x10\x00\x00\x00"
                                 "branch: default\n",
                                 21),
            .expected_output =
                HgProtocol::HgOutput{true, HgProtocol::HgChannel::Output,
                                     "branch: default\n"}},
        HgOutputTestParameters{
            .name = "Input channel",
            .input = std::string("I\xFF\x00\x00\x00", 5),
            .expected_output =
                HgProtocol::HgOutput{false, HgProtocol::HgChannel::Input, ""}},
        HgOutputTestParameters{.name = "Line channel",
                               .input = std::string("L", 1),
                               .expected_output = HgProtocol::HgOutput{
                                   false, HgProtocol::HgChannel::Line, ""}}));
