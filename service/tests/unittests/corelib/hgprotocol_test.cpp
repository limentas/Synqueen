#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <list>
#include <memory>
#include <string>

#include "corelib/src/patch/hgprotocol.hpp"

using namespace testing;
using namespace std;
using namespace std::string_literals;
using namespace synqueen;

TEST(HgProtocolTest, CtorDtor) {
  HgProtocol protocol;
  auto hello = protocol.getHelloMessage();
  EXPECT_EQ(hello.capabilities, "");
  EXPECT_EQ(hello.version, "");
  EXPECT_EQ(hello.encoding, "");
  EXPECT_EQ(hello.pid, 0);
}

TEST(HgProtocolTest, ParseHelloMessage) {
  HgProtocol protocol;
  auto hello = "o\x00\x00\x00\x3F"
               "capabilities: runcommand getencoding\n"
               "encoding: UTF-8\n"
               "pid: 12345"s;
  auto result = protocol.feedStdOutput(hello.c_str(), hello.size());
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(protocol.getHelloMessage().capabilities, "runcommand getencoding");
  EXPECT_EQ(protocol.getHelloMessage().encoding, "UTF-8");
  EXPECT_EQ(protocol.getHelloMessage().pid, 12345);
}

TEST(HgProtocolTest, FeedStdOutput) {
  HgProtocol protocol;
  auto hello = "o\x00\x00\x00\x3F"
               "capabilities: runcommand getencoding\n"
               "encoding: UTF-8\n"
               "pid: 12345"s;
  auto result = protocol.feedStdOutput(hello.c_str(), hello.size());
  EXPECT_FALSE(result.has_value());

  // Check that the parser accepts partial input
  auto input = "o\x00\x00\x00\x1B"
               "parent: 14571:17c0cb1045e5\n"
               "o\x00\x00"s;
  result = protocol.feedStdOutput(input.c_str(), input.size());
  EXPECT_FALSE(result.has_value());

  input = "\x00\x03"
          "tip"
          "r\x00\x00\x00\x04"
          "\x00\x00\x00\x00"s;
  result = protocol.feedStdOutput(input.c_str(), input.size());
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(result->output, "parent: 14571:17c0cb1045e5\ntip");
  EXPECT_EQ(result->resultCode, 0);
}

TEST(HgProtocolTest, Reset) {
  HgProtocol protocol;
  auto hello = "o\x00\x00\x00\x3F"
               "capabilities: runcommand getencoding\n"
               "encoding: UTF-8\n"
               "pid: 12345"s;
  auto result = protocol.feedStdOutput(hello.c_str(), hello.size());
  EXPECT_FALSE(result.has_value());
  auto input = "o\x00\x00\x00\x1B"
               "parent: 14571:17c0cb1045e5\n"
               "o\x00\x00"s;
  result = protocol.feedStdOutput(input.c_str(), input.size());
  EXPECT_FALSE(result.has_value());

  protocol.reset();
  // Reset should clear hello message
  EXPECT_EQ(protocol.getHelloMessage().capabilities, "");
  EXPECT_EQ(protocol.getHelloMessage().encoding, "");
  EXPECT_EQ(protocol.getHelloMessage().pid, 0);

  // The internal buffer should be cleared
  input = "o\x00\x00\x00\x34"
          "capabilities: runcommand getencoding\n"
          "encoding: UTF-8"
          "o\x00\x00\x00\x1B"
          "parent: 14571:17c0cb1045e5\n"
          "r\x00\x00\x00\x04"
          "\x00\x00\x00\x00"s;
  result = protocol.feedStdOutput(input.c_str(), input.size());
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(result->output, "parent: 14571:17c0cb1045e5\n");
  EXPECT_EQ(result->resultCode, 0);
}

// To test HgProtocol::prepareCommand
struct PrepareCommandTestParameters {
  std::string name;
  std::list<std::string> input;

  std::string expectedOutput;
};

inline void PrintTo(const PrepareCommandTestParameters &p, ::std::ostream *os) {
  *os << "{ name=" << p.name << " }";
}

class PrepareCommandTest
    : public ::testing::TestWithParam<PrepareCommandTestParameters> {};

TEST_P(PrepareCommandTest, PrepareCommand) {
  const auto &param = GetParam();
  HgProtocol protocol;
  std::string actualOutput = protocol.prepareCommand(param.input);
  EXPECT_EQ(actualOutput, param.expectedOutput);
}

INSTANTIATE_TEST_SUITE_P(
    , PrepareCommandTest,
    // TODO: Make sure that "" ""s works for other compilers,
    // otherwise switch to ""s ""s
    testing::Values(
        PrepareCommandTestParameters{.name = "Docs log example",
                                     .input = {"log", "-l", "5"},
                                     .expectedOutput = "runcommand\n"
                                                       "\x00\x00\x00\x08"
                                                       "log\0"
                                                       "-l\0"
                                                       "5"s},
        PrepareCommandTestParameters{.name = "Docs summary example",
                                     .input = {"summary"},
                                     .expectedOutput = "runcommand\n"
                                                       "\x00\x00\x00\x07"
                                                       "summary"s},
        PrepareCommandTestParameters{.name = "Docs import example",
                                     .input = {"import", "-"},
                                     .expectedOutput = "runcommand\n"
                                                       "\x00\x00\x00\x08"
                                                       "import\0"
                                                       "-"s}));

// To test HgProtocol::feedStdOutput
struct FeedStdOutputTestParameters {
  std::string name;
  std::string input;

  std::optional<HgProtocol::CommandResult> expectedResult;
};

inline void PrintTo(const FeedStdOutputTestParameters &p, ::std::ostream *os) {
  *os << "{ name=" << p.name << " }";
}

class FeedStdOutputTest
    : public ::testing::TestWithParam<FeedStdOutputTestParameters> {};

TEST_P(FeedStdOutputTest, FeedStdOutput) {
  const auto &param = GetParam();
  HgProtocol protocol;
  auto actualResult =
      protocol.feedStdOutput(param.input.c_str(), param.input.size());
  EXPECT_EQ(actualResult.has_value(), param.expectedResult.has_value());
  if (param.expectedResult.has_value()) {
    EXPECT_EQ(actualResult->output, param.expectedResult->output);
    EXPECT_EQ(actualResult->resultCode, param.expectedResult->resultCode);
    EXPECT_EQ(actualResult->requiresInput, param.expectedResult->requiresInput);
    EXPECT_EQ(actualResult->requiredInputSize,
              param.expectedResult->requiredInputSize);
  }
}

INSTANTIATE_TEST_SUITE_P(
    , FeedStdOutputTest,
    testing::Values(
        FeedStdOutputTestParameters{.name = "Empty input",
                                    .input = ""s,
                                    .expectedResult = std::nullopt},
        FeedStdOutputTestParameters{.name = "No data length in capabilities",
                                    .input = "o"s,
                                    .expectedResult = std::nullopt},
        FeedStdOutputTestParameters{
            .name = "Not enough data in capabilities data length",
            .input = "o\x00\x00"s,
            .expectedResult = std::nullopt},
        FeedStdOutputTestParameters{.name = "Not enough capabilities data",
                                    .input = "o\x00\x00\x00\x05"
                                             "capa"s,
                                    .expectedResult = std::nullopt},
        FeedStdOutputTestParameters{
            .name = "Capabilities only",
            .input = "o\x00\x00\x00\x34"
                     "capabilities: runcommand getencoding\n"
                     "encoding: UTF-8"s,
            .expectedResult = std::nullopt},
        FeedStdOutputTestParameters{
            .name = "Not enough data in command result data length",
            .input = "o\x00\x00\x00\x34"
                     "capabilities: runcommand getencoding\n"
                     "encoding: UTF-8"
                     "o\x00\x00\x00"s,
            .expectedResult = std::nullopt},
        FeedStdOutputTestParameters{
            .name = "Not enough command output data",
            .input = "o\x00\x00\x00\x34"
                     "capabilities: runcommand getencoding\n"
                     "encoding: UTF-8"
                     "o\x00\x00\x00\x1B"
                     "parent: 14"s,
            .expectedResult = std::nullopt},
        FeedStdOutputTestParameters{
            .name = "Check result code",
            .input = "o\x00\x00\x00\x34"
                     "capabilities: runcommand getencoding\n"
                     "encoding: UTF-8"
                     "o\x00\x00\x00\x00"
                     "r\x00\x00\x00\x04"
                     "\x00\x00\x00\x10\n"s,
            .expectedResult = HgProtocol::CommandResult{"", "", "", 16, false,
                                                        0}},
        FeedStdOutputTestParameters{
            .name = "Docs example",
            .input = "o\x00\x00\x00\x34"
                     "capabilities: runcommand getencoding\n"
                     "encoding: UTF-8"
                     "o\x00\x00\x00\x1B"
                     "parent: 14571:17c0cb1045e5\n"
                     "o\x00\x00\x00\x03"
                     "tip"
                     "o\x00\x00\x00\x01"
                     "\n"
                     "o\x00\x00\x00\x34"
                     "paper, coal: display diffstat on the changeset page\n"
                     "o\x00\x00\x00\x10"
                     "branch: default\n"
                     "o\x00\x00\x00\x10"
                     "commit: (clean)\n"
                     "o\x00\x00\x00\x12"
                     "update: (current)\n"
                     "r\x00\x00\x00\x04"
                     "\x00\x00\x00\x00"s,
            .expectedResult =
                HgProtocol::CommandResult{
                    "parent: 14571:17c0cb1045e5\n"
                    "tip\n"
                    "paper, coal: display diffstat on the changeset page\n"
                    "branch: default\n"
                    "commit: (clean)\n"
                    "update: (current)\n",
                    "", "", 0, false, 0}},
        FeedStdOutputTestParameters{
            .name = "Another docs example with input channel activity",
            .input = "o\x00\x00\x00\x34"
                     "capabilities: runcommand getencoding\n"
                     "encoding: UTF-8"
                     "o\x00\x00\x00\x1A"
                     "applying patch from stdin\n"
                     "I\x00\x00\x10\x00"s,
            .expectedResult = HgProtocol::CommandResult{
                "applying patch from stdin\n", "", "", 0, true, 4096}}));
