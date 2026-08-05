#pragma once

#include "cob/process_exec.hpp"

namespace catalyst {
/**
 * @brief Executes a subprocess.
 *
 * @param args Linux Backend to process_exec (intead of reproc)
 * @param working_dir Optional working directory for the subprocess.
 * @param env Optional environment variables to extend/override the parent environment.
 * @param capture_output If true, captures and returns the combined stdout and stderr of the process.
 * @return On success, the process exit code and captured output (if requested).
 *         On failure to execute or monitor the process, an error message.
 */
Result<std::pair<int, std::string>>
processExecLinux(const std::vector<std::string> &args,
                   std::optional<std::string> working_dir = std::nullopt,
                   std::optional<std::vector<std::pair<std::string, std::string>>> env = std::nullopt,
                   bool capture_output = false);
} // namespace catalyst
