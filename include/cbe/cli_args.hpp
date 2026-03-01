#pragma once

#include "cbe/executor.hpp"

namespace catalyst {
struct CliArgs {
    catalyst::ExecutorConfig config;
    bool compdb = false;
    bool graph = false;
    std::filesystem::path work_dir = ".";
    std::vector<std::pair<std::string_view, std::string_view>> definition_overrides;
};

catalyst::Result<CliArgs> cliArgs(int argc, const char *const *argv);
void printHelp();
void printVersion();
} // namespace catalyst
