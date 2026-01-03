#include "cbe/executor.hpp"

#include "cbe/builder.hpp"

#include <algorithm>
#include <filesystem>
#include <format>
#include <print>

namespace catalyst {

bool file_changed(const std::filesystem::path &input_file, const auto &out_mod_time) {
    return std::filesystem::last_write_time(input_file) >= out_mod_time;
}

Executor::Executor(CBEBuilder &&builder) : builder(std::move(builder)) {
}

Result<void> Executor::execute() {
    catalyst::BuildGraph build_graph = builder.emit_graph();

    std::vector<size_t> build_order;
    if (auto res = build_graph.topo_sort(); !res) {
        return std::unexpected(res.error());
    } else {
        build_order = *res;
    }

    const auto &defs = builder.definitions();
    auto get_def = [&](std::string_view key) -> std::string {
        if (auto it = defs.find(key); it != defs.end())
            return std::string(it->second);
        return "";
    };

    const std::string cc = get_def("cc");
    const std::string cxx = get_def("cxx");
    const std::string cxxflags = get_def("cxxflags");
    const std::string cflags = get_def("cflags");
    const std::string ldflags = get_def("ldflags");
    const std::string ldlibs = get_def("ldlibs");

    for (size_t node_idx : build_order) {
        const auto &node = build_graph.nodes()[node_idx];
        if (node.step_id.has_value()) {
            const auto &step = build_graph.steps()[*node.step_id];

            std::vector<std::string> inputs;
            std::string_view pending = step.inputs;
            while (true) {
                size_t pos = pending.find(',');
                if (pos == std::string_view::npos) {
                    if (!pending.empty())
                        inputs.emplace_back(pending);
                    break;
                }
                inputs.emplace_back(pending.substr(0, pos));
                pending.remove_prefix(pos + 1);
            }

            std::string inputs_str;
            for (const auto &input : inputs) {
                if (!inputs_str.empty())
                    inputs_str += " ";
                inputs_str += input;
            }

            std::string command_string;
            bool needs_rebuild = false;

            if (std::filesystem::exists(step.output)) {
                auto output_modtime = std::filesystem::last_write_time(step.output);
                for (const auto &input : inputs) {
                    if (file_changed(input, output_modtime)) {
                        needs_rebuild = true;
                        break;
                    }
                }
            } else {
                needs_rebuild = true;
            }

            if (needs_rebuild) {
                if (step.tool == "cc") {
                    command_string = std::format(
                        "{} {} -MMD -MF {}.d -c {} -o {}", cc, cflags, step.output, inputs_str, step.output);
                } else if (step.tool == "cxx") {
                    command_string = std::format(
                        "{} {} -MMD -MF {}.d -c {} -o {}", cxx, cxxflags, step.output, inputs_str, step.output);
                } else if (step.tool == "ld") {
                    command_string = std::format("{} {} -o {} {} {}", cxx, inputs_str, step.output, ldflags, ldlibs);
                } else if (step.tool == "ar") {
                    command_string = std::format("ar rcs {} {}", step.output, inputs_str);
                } else if (step.tool == "sld") {
                    command_string = std::format("{} -shared {} -o {}", cxx, inputs_str, step.output);
                }
                std::println("{}", command_string);
                std::system(command_string.c_str());
            }
        }
    }
    return {};
}

} // namespace catalyst
