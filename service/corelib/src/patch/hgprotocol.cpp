#include "hgprotocol.hpp"

#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace synqueen {

void HgProtocol::parseHgOutput(const char *buffer, size_t length,
                               HgOutput &hgOutput) {
  spdlog::info("Received from hg cmdserver: {}",
               spdlog::to_hex(buffer, buffer + length));

  if (length < 1) {
    throw std::runtime_error("Invalid hg output: empty buffer");
  }

  char channelChar = buffer[0];
  if (std::isupper(static_cast<unsigned char>(channelChar))) {
    // The channels can be required and their identifiers are uppercase letters
    switch (channelChar) {
    case 'I':
      // Input channel is for hg to request exact size bytes input from the user
      hgOutput.channel = HgChannel::Input;
      break;
    case 'L':
      // Line based input channel is for hg to request one single line input
      // from the user
      hgOutput.channel = HgChannel::Line;
      break;
    default:
      // If the channel is not recognized, we have to abort execution
      throw std::runtime_error(
          std::string("Invalid hg output: unknown required channel '") +
          channelChar + "'");
    }
    hgOutput.hasData = false;
    return;
  }

  // The channels can be optional and their identifiers are lowercase letters
  if (length < 5) {
    throw std::runtime_error("Invalid hg output: too short");
  }

  switch (channelChar) {
  case 'o':
    hgOutput.channel = HgChannel::Output;
    break;
  case 'e':
    hgOutput.channel = HgChannel::Error;
    break;
  case 'r':
    hgOutput.channel = HgChannel::Result;
    break;
  case 'd':
    hgOutput.channel = HgChannel::Debug;
    break;
  default:
    // Unknown optional channels can be ignored
    hgOutput.hasData = false;
    return;
  }
  auto dataLength = *reinterpret_cast<const uint32_t *>(buffer + 1);
  if (length < 5 + dataLength) {
    throw std::runtime_error("Invalid hg output: data length mismatch");
  }
  // TODO: Handle different endianness and alignment issues if necessary
  hgOutput.data.assign(buffer + 5, dataLength);
  hgOutput.hasData = true;
}

} // namespace synqueen
