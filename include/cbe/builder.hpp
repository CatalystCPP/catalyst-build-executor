#pragma once

#include "cbe/graph.hpp"

#include <filesystem>
#include <memory>

namespace catalyst {

class CBEBuilder {
public:
    Result<void> add_step(BuildStep &&bs);

    const BuildGraph &graph() const {
        return graph_;
    }
    BuildGraph &&emit_graph() {
        return std::move(graph_);
    }

    void add_definition(std::string_view key, std::string_view value) {
        definitions_.emplace(key, value);
    }

    void add_resource(std::shared_ptr<void> res) {
        graph_.add_resource(std::move(res));
    }

    const Definitions &definitions() const {
        return definitions_;
    }

    friend Result<void> parse(CBEBuilder &, const std::filesystem::path &);
    friend Result<void> parse_bin(CBEBuilder &);
    friend Result<void> emit_bin(CBEBuilder &);

private:
    BuildGraph graph_;
    Definitions definitions_;
};

} // namespace catalyst
