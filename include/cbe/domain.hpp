#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace catalyst {

struct BuildStep {
    std::string_view tool;
    std::string_view inputs; // Comma-separated list
    std::string_view output;
};

using Definitions = std::unordered_map<std::string_view, std::string_view>;

} // namespace catalyst
