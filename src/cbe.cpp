#include "cbe/builder.hpp"
#include "cbe/cli_args.hpp"
#include "cbe/executor.hpp"
#include "cbe/parser.hpp"

#include <filesystem>
#include <format>
#include <iostream>
#include <print>
#include <string_view>
#include <utility>

int main(const int argc, const char *const *argv) {
    auto res = catalyst::cliArgs(argc, argv);
    if (!res) {
        if (res.error() != "") {
            std::cerr << res.error() << '\n';
            return 1;
        }
        return 0;
    }
    const auto [config, compdb, graph, commands, work_dir, definition_overrides] = *res;

    if (work_dir != ".") {
        std::error_code ec;
        std::filesystem::current_path(work_dir, ec);
        if (ec) {
            std::println(std::cerr, "Failed to change directory to {}: {}", work_dir.string(), ec.message());
            return 1;
        }
    }

    catalyst::CBEBuilder builder;

    if (!std::filesystem::exists(config.build_file)) {
        std::println(std::cerr, "Build File: {} does not exist.", config.build_file);
        return 1;
    }

    if (auto res = catalyst::parse(builder, config.build_file); !res) {
        std::println(std::cerr, "Failed to parse: {}", res.error());
        return 1;
    }

    for (const auto &[variable, value] : definition_overrides) {
        builder.add_definition(variable, value);
    }

    catalyst::Executor executor{std::move(builder), config};

    if (compdb) {
        auto _ = executor.emit_compdb();
    } else if (graph) {
        auto _ = executor.emit_graph();
    } else if (commands) {
        auto _ = executor.emit_commands();
    } else if (config.clean) {
        if (auto res = executor.clean(); !res) {
            std::println(std::cerr, "Clean failed: {}", res.error());
            return 1;
        }
    } else if (auto res = executor.execute(); !res) {
        std::println(std::cerr, "Execution failed: {}", res.error());
        return 1;
    }

    return 0;
}
