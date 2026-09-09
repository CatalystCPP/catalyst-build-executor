// Copyright 2026 Siddharth Mohanty
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "cob/utility.hpp"

#include <span>
#include <string>
#include <utility>

namespace catalyst::detail {
[[nodiscard]] inline Result<void>
validateEnvironment(std::span<const std::pair<std::string, std::string>> environment) {
    for (const auto &[key, value] : environment) {
        if (key.empty() || key.contains('=') || key.contains('\0')) {
            return std::unexpected(std::format("Invalid environment variable key: '{}'", key));
        }
        if (value.contains('\0')) {
            return std::unexpected(std::format("Invalid environment variable value for key '{}'", key));
        }
    }
    return {};
}
} // namespace catalyst::detail
