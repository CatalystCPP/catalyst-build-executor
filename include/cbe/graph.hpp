#pragma once

#include "cbe/domain.hpp"
#include "cbe/utility.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace catalyst {

class BuildGraph {
public:
    struct Node {
        std::string_view path;
        std::vector<size_t> out_edges;
        std::optional<size_t> step_id;
    };

    size_t get_or_create_node(std::string_view path);
    Result<size_t> add_step(BuildStep step);

    // Keep resources alive as long as the graph is alive
    void add_resource(std::shared_ptr<void> res) {
        resources_.push_back(std::move(res));
    }

    const std::vector<Node> &nodes() const {
        return nodes_;
    }
    const std::vector<BuildStep> &steps() const {
        return steps_;
    }

    Result<std::vector<size_t>> topo_sort() const;

private:
    std::vector<Node> nodes_;
    std::vector<BuildStep> steps_;
    std::unordered_map<std::string_view, size_t> index_;
    std::vector<std::shared_ptr<void>> resources_;
};

} // namespace catalyst
