#include "cloud/rclonerpc.hpp"
#include "gtestprinters.hpp"
#include "testutils.hpp"
#include "utils/continuationscheduler.hpp"
#include "utils/corraleventlooptraits.hpp"
#include "utils/standardpaths.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidjson/error/en.h>

using namespace testing;
using namespace std;
using namespace std::string_literals;
using namespace synqueen;
using namespace synqueen::cloud;
using namespace synqueen::utils;

class RcloneRpcTest : public testing::Test {
protected:
  void SetUp() override { StandardPaths::initializeInTemp("Synqueen-test"); }
  void TearDown() override { cleanupAppFolders(); }
};

TEST_F(RcloneRpcTest, CtorDtor) {
  auto l = new uv_loop_t();
  auto result = uv_loop_init(l);
  EXPECT_EQ(result, 0);
  auto loop = LoopPtr(l, deleteLoop);
  RcloneRpc rcloneRpc(*loop);
}

TEST_F(RcloneRpcTest, Shutdown) {
  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);
  corral::run(*loop, [&loop]() -> corral::Task<void> {
    RcloneRpc rcloneRpc(*loop);
    co_await rcloneRpc.shutdown();
    co_return;
  });
}

TEST_F(RcloneRpcTest, RunCommand) {
  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);
  corral::run(*loop, [&loop]() -> corral::Task<void> {
    RcloneRpc rcloneRpc(*loop);
    ContinuationScheduler::initialize(*loop);
    // These samples are taken from
    // https://github.com/rclone/rclone/blob/master/librclone/ctest/ctest.c
    json::Document input;
    input.Parse("{"
                " \"p1\": [1,\"2\",null,4],"
                " \"p2\": { \"a\":1, \"b\":2 } "
                "}");
    json::CrtAllocator crt;
    json::GenericDocument<json::UTF8<>, json::CrtAllocator> inputCopy;
    inputCopy.CopyFrom(input, crt);
    auto res = co_await rcloneRpc.runCommand("rc/noop", std::move(input));
    EXPECT_THAT(res.error, IsEmpty());
    EXPECT_EQ(*res.output, inputCopy);

    input = json::Document();
    input.Parse("{"
                " \"p1\": [1,\"2\",null,4],"
                " \"p2\": { \"a\":1, \"b\":2 } "
                "}");
    inputCopy.CopyFrom(input, crt);
    res = co_await rcloneRpc.runCommand("rc/error", std::move(input));
    EXPECT_THAT(res.error, Not(IsEmpty()));

    input = json::Document();
    input.Parse("{}");
    res = co_await rcloneRpc.runCommand("config/listremotes", std::move(input));
    EXPECT_THAT(res.error, IsEmpty());
    // output is empty, but we don't care about its content here

    json::Document output;
    output.Parse("{ \"remotes\": [] }");
    EXPECT_EQ(*res.output, output);

    co_await rcloneRpc.shutdown();
    co_return;
  });
}

TEST_F(RcloneRpcTest, CreateRemote) {
  auto l = new uv_loop_t();
  auto r = uv_loop_init(l);
  EXPECT_EQ(r, 0);
  auto loop = LoopPtr(l, deleteLoop);
  corral::run(*loop, [&loop]() -> corral::Task<void> {
    RcloneRpc rcloneRpc(*loop);
    ContinuationScheduler::initialize(*loop);
    json::Document input;
    input.SetObject().AddMember("configPassword", "password123",
                                input.GetAllocator());
    auto res = co_await rcloneRpc.runCommand("config/unlock", std::move(input));

    // TODO: Store the secrets in a more secure way
    auto c = "{"
             " \"name\": \"test-remote\","
             " \"type\": \"drive\","
             " \"parameters\": {"
             "   \"client_id\": \"" GDRIVE_CLIENT_ID "\","
             "   \"client_secret\": \"" GDRIVE_CLIENT_SECRET "\","
             "   \"config_is_local\": true,"
             "   \"description\": \"Test remote2\","
             "   \"skip_gdocs\": true,"
             "   \"scope\": \"drive.file\","
             "   \"skip_checksum_gphotos\": true,"
             "   \"config_change_team_drive\": false"
             " },"
             " \"opt\": {"
             "   \"noOutput\": true,"
             "   \"nonInteractive\": true"
             " }"
             "}";
    input = json::Document();
    input.Parse(c);
    EXPECT_FALSE(input.HasParseError())
        << json::GetParseError_En(input.GetParseError());
    json::CrtAllocator crt;
    json::GenericDocument<json::UTF8<>, json::CrtAllocator> inputCopy;
    inputCopy.CopyFrom(input, crt);
    res = co_await rcloneRpc.runCommand("config/create", std::move(input));
    EXPECT_THAT(res.error, IsEmpty());
    EXPECT_EQ(*res.output, input);

    co_await rcloneRpc.shutdown();
    co_return;
  });
}
