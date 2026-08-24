#include "hgprotocol.hpp"

#include <bit>
#include <ranges>
#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

#include "utils/utils.hpp"

using namespace std;
using namespace std::string_literals;

namespace synqueen {

HgProtocol::HgProtocol() { buffer.reserve(10'240); }

string HgProtocol::prepareCommand(const list<string> &args) const {
  // Example: for 'hg log -l 5', the command is sent as:
  // runcommand\n
  // 8
  // log\0
  // -l\0
  // 5
  if (args.empty()) {
    throw invalid_argument("Command parts cannot be empty");
  }
  size_t length = 0;
  for (const auto &part : args) {
    length += part.size() + 1; // +1 for the null terminator
  }
  length -= 1; // Remove the last null terminator

  string preparedCommand;
  preparedCommand.reserve(runCommandLength + length + sizeof(uint32_t));
  preparedCommand += runCommand;
  auto length32 = toBigEndian(static_cast<uint32_t>(length));
  preparedCommand.append(reinterpret_cast<const char *>(&length32),
                         sizeof(length32));
  for (const auto &part : args) {
    preparedCommand += part;
    preparedCommand += '\0';
  }
  preparedCommand.pop_back(); // Remove the last null terminator
  return preparedCommand;
}

optional<HgProtocol::CommandResult> HgProtocol::feedStdOutput(const char *data,
                                                              size_t length) {
  spdlog::info("Received from hg cmdserver: {}", toPrintable(data, length));
  buffer.append(data, length);
  if (buffer.size() < 1) {
    return nullopt;
  }

  if (!tryParseHelloMessage()) {
    return nullopt;
  }

  CommandResult result;
  string_view bufferView(buffer);
  while (!bufferView.empty()) {
    char channelChar = bufferView[0];
    bufferView.remove_prefix(1);
    if (isupper(static_cast<unsigned char>(channelChar))) {
      // The channels can be required and their identifiers are uppercase
      // letters
      switch (channelChar) {
      case 'I':
        // Input channel is for hg to request exact size bytes input from the
        // user
        if (bufferView.size() < 4) {
          return nullopt;
        }
        result.requiresInput = true;
        result.requiredInputSize = fromBigEndian(bufferView.data());
        bufferView.remove_prefix(4);
        // Remove the processed part from the buffer
        buffer.erase(0, buffer.size() - bufferView.size());
        return result;
      case 'L':
        // Line based input channel is for hg to request one single line input
        // from the user
        // TODO: Implement if necessary
        return CommandResult{};
      default:
        // If the channel is not recognized, we have to abort execution
        throw runtime_error(
            string("Invalid hg output: unknown required channel '") +
            channelChar + "'");
      }
    }

    // The channels can be optional and their identifiers are lowercase letters
    if (bufferView.size() < 4) {
      return nullopt;
    }

    auto dataLength = fromBigEndian(bufferView.data());
    if (bufferView.size() < 4 + dataLength) {
      return nullopt;
    }
    bufferView.remove_prefix(4);

    switch (channelChar) {
    case 'o':
      result.output += string(bufferView.data(), dataLength);
      bufferView.remove_prefix(dataLength);
      break;
    case 'e':
      result.error += string(bufferView.data(), dataLength);
      bufferView.remove_prefix(dataLength);
      break;
    case 'd':
      result.debug += string(bufferView.data(), dataLength);
      bufferView.remove_prefix(dataLength);
      break;
    case 'r':
      assert(dataLength == 4);
      result.resultCode = fromBigEndian(bufferView.data());
      bufferView.remove_prefix(4);
      // Remove the processed part from the buffer
      buffer.erase(0, buffer.size() - bufferView.size());
      return result;
    default:
      // Unknown optional channels can be ignored
      bufferView.remove_prefix(dataLength);
      break;
    }
  }
  return std::nullopt;
}

void HgProtocol::reset() {
  readState = ReadState::Capabilities;
  helloMessage = HelloMessage{};
  buffer.clear();
}

bool HgProtocol::tryParseHelloMessage() {
  if (readState != ReadState::Capabilities) {
    return true;
  }
  // The first message from the hg cmdserver is the hello message sent to 'o'
  // channel
  if (buffer.size() < 5) {
    return false;
  }

  string_view bufferView(buffer);
  assert(bufferView[0] == 'o');
  bufferView.remove_prefix(1);
  auto dataLength = fromBigEndian(bufferView.data());
  if (bufferView.size() < 4 + dataLength) {
    return false;
  }
  bufferView.remove_prefix(4);

  // The data is a set of `<field name>: <field data>` pairs separated by
  // newline
  for (const auto field :
       views::split(bufferView.substr(0, dataLength), '\n')) {
    if (field.empty()) { // Skip empty parts
      continue;
    }
    auto fieldView = string_view(field);
    auto colonPos = fieldView.find(": ");
    if (colonPos == string_view::npos) {
      throw runtime_error("Invalid hg hello message: missing colon");
    }
    auto fieldName = fieldView.substr(0, colonPos);
    auto fieldData = fieldView.substr(colonPos + 2);
    if (fieldName == "capabilities") {
      helloMessage.capabilities = string(fieldData);
    } else if (fieldName == "encoding") {
      helloMessage.encoding = string(fieldData);
    } else if (fieldName == "pid") {
      helloMessage.pid = stoi(string(fieldData));
    }
  }
  bufferView.remove_prefix(dataLength);

  readState = ReadState::CommandResult;
  buffer.erase(0, buffer.size() - bufferView.size());
  return true;
}

uint32_t HgProtocol::toBigEndian(uint32_t length) const {
  if constexpr (endian::native == endian::big)
    return length;
  return byteswap(length);
}

uint32_t HgProtocol::fromBigEndian(const char *data) const {
  auto *p = reinterpret_cast<const uint32_t *>(data);
  if constexpr (endian::native == endian::big)
    return *p;
  return byteswap(*p);
}

} // namespace synqueen
