#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <optional>
#include <stdexcept>
#include <iostream>

// Represents a single build step (tool + inputs + output).
struct BuildStep {
    std::string tool;
    std::vector<std::string> inputs;
    std::string output;
};

// In-memory build graph: nodes are files/paths; edges: input -> output.
class BuildGraph {
public:
    struct Node {
        std::string path;              // file path (input or output)
        std::vector<int> out_edges;    // neighbors that depend on this node
        std::optional<int> step_id;    // index into steps_ if this node is produced by a step
    };

    // Add a node or return existing index.
    int get_or_create_node(std::string_view path) {
        auto it = index_.find(path);
        if (it != index_.end())
            return it->second;

        int id = static_cast<int>(nodes_.size());
        nodes_.push_back(Node{std::string(path), {}, std::nullopt});
        index_.emplace(nodes_.back().path, id); // use stable storage
        return id;
    }

    // Register a build step, creating nodes and edges.
    int add_step(const BuildStep &step) {
        int out_id = get_or_create_node(step.output);

        // enforce "one producer per output"
        if (nodes_[out_id].step_id.has_value()) {
            throw std::runtime_error("Duplicate producer for output: " + step.output);
        }

        int step_id = static_cast<int>(steps_.size());
        steps_.push_back(step);
        nodes_[out_id].step_id = step_id;

        // edges: input -> output
        for (const auto &in : step.inputs) {
            int in_id = get_or_create_node(in);
            nodes_[in_id].out_edges.push_back(out_id);
        }

        return step_id;
    }

    const std::vector<Node>& nodes() const { return nodes_; }
    const std::vector<BuildStep>& steps() const { return steps_; }

    // DFS-based topological sort: returns a *total* ordering of all nodes.
    std::vector<int> topo_sort() const {
        enum class Color : uint8_t { White, Gray, Black };

        std::vector<Color> color(nodes_.size(), Color::White);
        std::vector<int> order;
        order.reserve(nodes_.size());

        // lambda DFS
        std::function<void(int)> dfs = [&](int u) {
            color[u] = Color::Gray;
            for (int v : nodes_[u].out_edges) {
                if (color[v] == Color::White) {
                    dfs(v);
                } else if (color[v] == Color::Gray) {
                    throw std::runtime_error("Cycle detected in build graph at: " + nodes_[v].path);
                }
            }
            color[u] = Color::Black;
            order.push_back(u);
        };

        for (int i = 0; i < static_cast<int>(nodes_.size()); ++i) {
            if (color[i] == Color::White) {
                dfs(i);
            }
        }

        // DFS finishes leaves first → reverse to get build order
        std::reverse(order.begin(), order.end());
        return order;
    }

private:
    std::vector<Node> nodes_;
    std::vector<BuildStep> steps_;
    std::unordered_map<std::string, int> index_; // path -> node id
};

// High-level builder: in real CBE this is what your manifest parser would drive.
class CBEBuilder {
public:
    // Example API: take tool + csv inputs + output.
    void add_step(std::string tool,
                  std::vector<std::string> inputs,
                  std::string output)
    {
        BuildStep s{
            .tool   = std::move(tool),
            .inputs = std::move(inputs),
            .output = std::move(output),
        };
        graph_.add_step(s);
    }

    const BuildGraph& graph() const { return graph_; }
    BuildGraph&& emit_graph() { return std::move(graph_); }

private:
    BuildGraph graph_;
};


int main() {
    CBEBuilder builder;

    // Imagine these came from:
    //   cxx|src/main.cpp|build/main.o
    //   cxx|src/util.cpp|build/util.o
    //   ld|build/main.o,build/util.o|build/app;
    builder.add_step("cxx", {"src/main.cpp"}, "build/main.o");
    builder.add_step("cxx", {"src/util.cpp"}, "build/util.o");
    builder.add_step("ld", {"build/main.o", "build/util.o"}, "build/app");

    BuildGraph graph = builder.emit_graph();

    auto order = graph.topo_sort();
    std::cout << "Topological build order:\n";
    for (int id : order) {
        std::cout << "  " << graph.nodes()[id].path;
        if (graph.nodes()[id].step_id.has_value()) {
            const auto &step = graph.steps()[*graph.nodes()[id].step_id];
            std::cout << "    (tool=" << step.tool << ")";
        }
        std::cout << "\n";
    }

    return 0;
}

