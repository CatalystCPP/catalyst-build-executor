#include "cbe/builder.hpp"
#include "cbe/executor.hpp"
#include "cbe/parser.hpp"

#include <charconv>
#include <cstring>
#include <filesystem>
#include <format>
#include <iostream>
#include <print>
#include <string>

void printHelp() {
    std::println("Usage: cbe [options]");
    std::println("Options:");
    std::println("  -h, --help       Show this help message");
    std::println("  -v, --version    Show version");
    std::println("  -d <dir>         Change working directory before doing anything");
    std::println("  -e <estimate>    Use <estimate> as the estimate file (default: catalyst.estimates)");
    std::println("  -f <file>        Use <file> as the build manifest (default: catalyst.build)");
    std::println("  -j, --jobs <N>   Set number of parallel jobs (default: auto)");
    std::println("  --dry-run        Print commands without executing them");
    std::println("  --clean          Remove build artifacts");
    std::println("  --compdb         Generate compile_commands.json");
    std::println("  --graph          Generate DOT graph of build");
}

void printVersion() {
    std::println("cbe {}", CATALYST_PROJ_VER);
}

struct CliArgs {
    catalyst::ExecutorConfig config;
    bool compdb = false;
    bool graph = false;
    std::string input_path = "catalyst.build";
    std::string estimates_file = "catalyst.estimates";
    std::filesystem::path work_dir = std::filesystem::current_path();
};

catalyst::Result<CliArgs> CLIArgs(int argc, const char * const *argv);

int main(const int argc, const char *const *argv) {
    auto res = CLIArgs(argc, argv);
    if (!res) {
        std::println(std::cerr, "{}", res.error());
        return 1;
    }
    const auto [config, compdb, graph, input_path, estimates_file, work_dir] = *res;

    if (work_dir != ".") {
        std::error_code ec;
        std::filesystem::current_path(work_dir, ec);
        if (ec) {
            std::println(std::cerr, "Failed to change directory to {}: {}", work_dir.string(), ec.message());
            return 1;
        }
    }

    catalyst::CBEBuilder builder;

    if (!std::filesystem::exists(input_path)) {
        std::println(std::cerr, "Build File: {} does not exist.", input_path);
        return 1;
    }
    if (std::filesystem::is_symlink(input_path)) {
        // FIXME: figure out __how__ to support.
        std::println(std::cerr, "cbe does not support parsing symbolically linked files.");
        return 1;
    }

    if (auto res = catalyst::parse(builder, input_path); !res) {
        std::println(std::cerr, "Failed to parse: {}", res.error());
        return 1;
    }

    catalyst::Executor executor{std::move(builder), config};

    if (compdb) {
        auto _ = executor.emit_compdb();
    } else if (graph) {
        auto _ = executor.emit_graph();
    } else if (config.clean) {
        if (auto res = executor.clean(); !res) {
            std::println(std::cerr, "Clean failed: {}", res.error());
            return 1;
        }
    } else {
        if (auto res = executor.execute(); !res) {
            std::println(std::cerr, "Execution failed: {}", res.error());
            return 1;
        }
    }

    return 0;
}

catalyst::Result<CliArgs> CLIArgs(const int argc, const char *const *argv) {
    CliArgs par;
    catalyst::ExecutorConfig config;

    std::filesystem::path work_dir = ".";
    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printHelp();
            return std::unexpected("");
        }
        if (arg == "-v" || arg == "--version") {
            printVersion();
            return std::unexpected("");
        }
        if (arg == "-d") {
            if (i + 1 < argc) {
                work_dir = argv[i + 1];
                i++;
            } else {
                return std::unexpected("Missing argument for -d");
            }
        } else if (arg == "-f") {
            if (i + 1 < argc) {
                par.input_path = argv[i + 1];
                par.config.build_file = par.input_path;
                i++;
            } else {
                return std::unexpected("Missing argument for -f");
            }
        } else if (arg == "-e") {
            if (i + 1 < argc) {
                par.estimates_file = argv[i + 1];
                par.config.estimates_file = par.estimates_file;
                i++;
            } else {
                return std::unexpected("Missing argument for -e");
            }
        } else if (arg == "--dry-run") {
            par.config.dry_run = true;
        } else if (arg == "--clean") {
            par.config.clean = true;
        } else if (arg == "--compdb") {
            par.compdb = true;
        } else if (arg == "--graph") {
            par.graph = true;
        } else if (arg == "-j" || arg == "--jobs") {
            if (i + 1 < argc) {
                size_t jobs = 0;
                auto res = std::from_chars(argv[i + 1], argv[i + 1] + strlen(argv[i + 1]), jobs);
                if (res.ec == std::errc()) {
                    par.config.jobs = jobs;
                    i++;
                } else {
                    return std::unexpected(std::format("Invalid job count: {}", argv[i + 1]));
                }
            } else {
                return std::unexpected(std::format("Missing argument for {}", arg));
            }
        } else {
            return std::unexpected(std::format("Unknown argument: {}. Run {} --help for more information.", argv[0], arg));
        }
    }
    return par;
}
