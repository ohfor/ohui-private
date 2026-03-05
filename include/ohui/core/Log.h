#pragma once

#include <spdlog/spdlog.h>

namespace ohui::log {

// Thin wrappers around spdlog for core code diagnostics.
// Core code may use debug() and trace() for development diagnostics.
// Core code never calls logger::info/warn/error — those are
// boundary-layer concerns.

template<typename... Args>
void debug(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    spdlog::debug(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void trace(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    spdlog::trace(fmt, std::forward<Args>(args)...);
}

}  // namespace ohui::log
