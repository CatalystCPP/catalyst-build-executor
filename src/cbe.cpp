#include "cbe/builder.hpp"
#include "cbe/parser.hpp"

#include <filesystem>
#include <iostream>

int main(const int argc, const char *const *argv) {
    catalyst::CBEBuilder builder;
    std::filesystem::path input_path = "catalyst.build";

    if (auto res = catalyst::parse(builder, input_path); !res) {
        std::println(std::cerr, "Failed to parse: {}", res.error());
        return 1;
    }

    catalyst::BuildGraph graph = builder.emit_graph();

    std::vector<size_t> order;

    if (auto res = graph.topo_sort(); !res) {
        std::cerr << "Error: " << res.error() << "\n";
        return 1;
    } else {
        order = *res;
    }

    for (size_t id : order) {
        const auto &node = graph.nodes()[id];
        std::cout << "  " << node.path;
        if (node.step_id.has_value()) {
            const auto &step = graph.steps()[*node.step_id];
            std::cout << "    (tool=" << step.tool << ")";
        }
        std::cout << "\n";
    }

    return 0;
}
