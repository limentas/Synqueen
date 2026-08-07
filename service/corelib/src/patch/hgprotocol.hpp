#pragma once

#include <cstddef>
#include <cstdint>
#include <list>
#include <optional>
#include <string>

namespace synqueen {

class HgProtocol {
public:
  struct HelloMessage {
    std::string capabilities;
    std::string version;
    std::string encoding;
    int pid = 0;
  };
  struct CommandResult {
    std::string output; //< Joined content of output channel
    std::string error;  //< Joined content of error channel
    std::string debug;  //< Joined content of debug channel
    int resultCode = 0;
    bool requiresInput = false;
    int requiredInputSize = 0;
  };

  HgProtocol();
  std::string prepareCommand(const std::list<std::string> &command) const;
  std::optional<CommandResult> feedStdOutput(const char *data, size_t length);

  inline HelloMessage getHelloMessage() const { return helloMessage; }
  void reset();

private:
  bool tryParseHelloMessage();
  uint32_t toBigEndian(uint32_t length) const;
  uint32_t fromBigEndian(const char *data) const;

private:
  enum class ReadState { Capabilities, CommandResult, WaitingInput };

  const std::string runCommand = "runcommand\n";
  const size_t runCommandLength = runCommand.size();

  ReadState readState = ReadState::Capabilities;
  HelloMessage helloMessage;
  std::string buffer;
};

} // namespace synqueen
