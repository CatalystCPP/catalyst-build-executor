#include "cbe/cli_args.hpp"

#include <cstring>
#include <print>

namespace catalyst {
Result<CliArgs> cliArgs(const int argc, const char *const *argv) {
    using std::format;
    using std::unexpected;

    CliArgs par;

    auto set_next_arg = [&argc, &argv](auto &ref, int &i) -> bool {
        if (i + 1 >= argc)
            return false;
        ref = argv[++i];
        return true;
    };

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printHelp();
            return unexpected("");
        }
        if (arg == "-v" || arg == "--version") {
            printVersion();
            return unexpected("");
        }
        if (arg == "-C") {
            if (!set_next_arg(par.work_dir, i))
                return unexpected(format("Missing argument for {}", arg));
            continue;
        }
        if (arg == "-f") {
            if (!set_next_arg(par.config.build_file, i))
                return unexpected(format("Missing argument for {}", arg));
            continue;
        }
        if (arg == "--estimates") {
            if (!set_next_arg(par.config.estimates_file, i))
                return unexpected(format("Missing argument for {}", arg));
            continue;
        }
        if (arg == "-j" || arg == "--jobs") {
            if (i + 1 >= argc) {
                return unexpected(format("Missing argument for {}", arg));
            }
            i++;
            auto res = std::from_chars(argv[i], argv[i] + strlen(argv[i]), par.config.jobs);
            if (res.ec != std::errc()) {
                return unexpected(format("Invalid job count: {}", argv[i]));
            }
            continue;
        }
        if (arg == "-n" || arg == "--dry-run") {
            par.config.dry_run = true;
        } else if (arg == "--clean") {
            par.config.clean = true;
        } else if (arg == "--compdb") {
            par.compdb = true;
        } else if (arg == "--graph") {
            par.graph = true;
        } else if (arg == "-s" || arg == "--silent") {
            par.config.silent = true;
        } else if (arg == "-k" || arg == "--keep-going") {
            par.config.keep_going = true;
        } else if (const std::string_view::iterator pos = std::ranges::find(arg, '='); pos != arg.end()) {
            par.definition_overrides.emplace_back(std::string_view(arg.begin(), pos),
                                                  std::string_view(pos + 1, arg.end()));
        } else {
            return unexpected(format("Unknown argument: {}. Run {} --help for more information.", arg, argv[0]));
        }
    }
    return par;
}

void printHelp() {
    using std::println;
    println("Usage: cbe [options]");
    println("Options:");
    println("  -h, --help                    Show this help message");
    println("  -v, --version                 Show version");
    println("  -C <dir>                      Change working directory before doing anything");
    println("  -f <file>                     Use <file> as the build manifest (default: catalyst.build)");
    println("  -j, --jobs <N>                Set number of parallel jobs (default: auto)");
    println("  -k, --keep-going              Continue the build after error (default: false)");
    println("  -n, --dry-run                 Print commands without executing them");
    println("  -s, --silent                  Suppress cli output, only print errors");
    println("  --clean                       Remove build artifacts");
    println("  --compdb                      Generate compile_commands.json");
    println("  --estimates <estimate>        Use <estimate> as the estimate file (default: catalyst.estimates)");
    println("  --graph                       Generate DOT graph of build");
}

void printVersion() {
    std::println("cbe {}", CATALYST_PROJ_VER);
}
} // namespace catalyst
