#pragma once

#include <stdexcept>
#include <string>

namespace synqueen {

class SqException : public std::runtime_error {
public:
  explicit SqException(const std::string &message)
      : std::runtime_error(message) {}
};

class SqDoesNotExist : public SqException {
public:
  explicit SqDoesNotExist(const std::string &message) : SqException(message) {}
};

class SqAlreadyUsed : public SqException {
public:
  explicit SqAlreadyUsed(const std::string &message) : SqException(message) {}
};

class SqPermissionDenied : public SqException {
public:
  explicit SqPermissionDenied(const std::string &message)
      : SqException(message) {}
};

class SqNotADirectory : public SqException {
public:
  explicit SqNotADirectory(const std::string &message) : SqException(message) {}
};

class SqUnknownError : public SqException {
public:
  explicit SqUnknownError(const std::string &message) : SqException(message) {}
};

} // namespace synqueen
