// Copyright 2026 Siddharth Mohanty
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "cob/utility.hpp"

#include <future>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
namespace catalyst {
/**
 * @brief Executes a subprocess.
 *
 * @param args The command line arguments (first argument is the executable).
 * @param working_dir Optional working directory for the subprocess.
 * @param env Optional environment variables to extend/override the parent environment.
 * @param capture_output If true, captures and returns the combined stdout and stderr of the process.
 * @return On success, the process exit code and captured output (if requested).
 *         On failure to execute or monitor the process, an error message.
 */
Result<std::pair<int, std::string>>
process_exec(const std::vector<std::string> &args,
             std::optional<std::string> working_dir = std::nullopt,
             std::optional<std::vector<std::pair<std::string, std::string>>> env = std::nullopt,
             bool capture_output = false);
} // namespace catalyst
