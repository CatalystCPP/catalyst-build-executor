// Copyright 2026 Siddharth Mohanty
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include <expected>
#include <format>
#include <string>

namespace catalyst {
template <typename Value_T> using Result = std::expected<Value_T, std::string>;
} // namespace catalyst
