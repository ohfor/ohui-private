#pragma once

#include <expected>
#include <string>

namespace ohui {

enum class ErrorCode {
    ParseError,
    FileNotFound,
    InvalidFormat,
    SizeLimitExceeded,
    DuplicateRegistration,
};

struct Error {
    ErrorCode code;
    std::string message;
};

template<typename T>
using Result = std::expected<T, Error>;

}  // namespace ohui
