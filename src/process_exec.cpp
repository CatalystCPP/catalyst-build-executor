// Copyright 2026 Siddharth Mohanty
// SPDX-License-Identifier: Apache-2.0

#include "cob/process_exec.hpp"

#if defined(__linux__)
#include "process_exec_linux.hpp"
#else
#include "cob/utility.hpp"

#include <expected>
#include <optional>
#include <reproc++/run.hpp>
#include <string>
#include <utility>
#include <vector>
#endif

namespace catalyst {
Result<std::pair<int, std::string>> process_exec(const std::vector<std::string> &args,
                                                 std::optional<std::string> working_dir,
                                                 std::optional<std::vector<std::pair<std::string, std::string>>> env,
                                                 bool capture_output) {
#if defined(__linux__)
    return processExecLinux(args, std::move(working_dir), std::move(env), capture_output);
#else
    if (args.empty()) {
        return std::unexpected("Cannot execute empty command");
    }

    reproc::options options;
    std::string captured;
    if (capture_output) {
        options.redirect.out.type = reproc::redirect::pipe;
        options.redirect.err.type = reproc::redirect::pipe;
    } else {
        options.redirect.out.type = reproc::redirect::parent;
        options.redirect.err.type = reproc::redirect::parent;
    }

    if (working_dir) {
        options.working_directory = working_dir->c_str();
    }

    std::vector<std::string> env_strings;
    std::vector<const char *> env_ptrs;
    if (env) {
        options.env.behavior = reproc::env::extend;
        for (const auto &[key, value] : *env) {
            std::string &s = env_strings.emplace_back();
            s.reserve(key.size() + 1 + value.size());
            std::format_to(std::back_inserter(s), "{}={}", key, value);
        }
        for (const std::string &s : env_strings) {
            env_ptrs.push_back(s.c_str());
        }
        env_ptrs.push_back(nullptr);
        options.env.extra = env_ptrs.data();
    }

    int status = 0;
    std::error_code ec;
    if (capture_output) {
        reproc::sink::string sink(captured);
        std::tie(status, ec) = reproc::run(args, options, sink, sink);
    } else {
        std::tie(status, ec) = reproc::run(args, options);
    }

    if (ec) {
        return std::unexpected(std::format("Failed to execute '{}': {}", args.front(), ec.message()));
    }
    return std::pair<int, std::string>{status, captured};
#endif
}
} // namespace catalyst
