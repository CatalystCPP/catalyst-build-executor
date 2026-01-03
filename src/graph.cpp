#include "cbe/graph.hpp"

#include "cbe/utility.hpp"

#include <algorithm>
#include <cstdint>
#include <format>
#include <functional>

namespace catalyst {

size_t BuildGraph::get_or_create_node(std::string_view path) {
    if (auto it = index_.find(path); it != index_.end()) {
        return it->second;
    }

    size_t id = nodes_.size();
    nodes_.push_back({path, {}, std::nullopt});
    index_.emplace(path, id);
    return id;
}

Result<size_t> BuildGraph::add_step(BuildStep step) {
    size_t out_id = get_or_create_node(step.output);

    if (nodes_[out_id].step_id.has_value()) { // 2 different steps create the same file.
        return std::unexpected(std::format("Duplicate producer for output: {}", step.output));
    }

    size_t step_id = steps_.size();
    steps_.push_back(step); // Store the step
    nodes_[out_id].step_id = step_id;

    // Iterate over comma-separated inputs
    std::string_view remaining = step.inputs;
    while (!remaining.empty()) {
        size_t comma_pos = remaining.find(',');
        std::string_view in_path;
        if (comma_pos == std::string_view::npos) {
            in_path = remaining;
            remaining = {};
        } else {
            in_path = remaining.substr(0, comma_pos);
            remaining = remaining.substr(comma_pos + 1);
        }

        // TODO: check for and dispatch depfile parsing logic (check extension)
        // TODO: check for and dispatch response file parsing logic (check extension)

        if (!in_path.empty()) {
            size_t in_id = get_or_create_node(in_path);
            nodes_[in_id].out_edges.push_back(out_id);
        }
    }

    return step_id;
}

Result<std::vector<size_t>> BuildGraph::topo_sort() const {
    enum class STATUS : uint8_t { UNSTARTED, WORKING, FINISHED };

    std::vector<STATUS> status(nodes_.size(), STATUS::UNSTARTED);
    std::vector<size_t> order;
    order.reserve(nodes_.size());

    std::function<Result<void>(size_t)> dfs = [&](size_t u) -> Result<void> {
        status[u] = STATUS::WORKING;
        for (size_t v : nodes_[u].out_edges) {
            if (status[v] == STATUS::UNSTARTED) {
                if (auto res = dfs(v); !res)
                    return res;
            } else if (status[v] == STATUS::WORKING) {
                return std::unexpected(std::format("Cycle detected in the build graph at: {}", nodes_[v].path));
            }
        }
        status[u] = STATUS::FINISHED;
        order.push_back(u);
        return {};
    };

    for (size_t i = 0; i < nodes_.size(); ++i) {
        if (status[i] == STATUS::UNSTARTED) {
            if (auto res = dfs(i); !res)
                return std::unexpected(res.error());
        }
    }

    std::reverse(order.begin(), order.end());
    return order;
}

} // namespace catalyst
