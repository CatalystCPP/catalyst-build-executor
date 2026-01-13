#pragma once

#include "cbe/mmap.hpp"

#include <charconv>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <string_view>
#include <unordered_map>

namespace catalyst {
class WorkEstimate {
public:
    explicit WorkEstimate(const std::filesystem::path &path_to_estimates);
    explicit WorkEstimate(std::filesystem::path &&path_to_estimates) : WorkEstimate(path_to_estimates) {
    }

    [[clang::always_inline]] size_t get_work_estimate(std::string_view path) {
        if (auto res = estimates.find(path); res != estimates.end()) {
            size_t val = 0;
            auto str = res->second;
            auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), val);
            if (ec == std::errc{}) {
                return val;
            }
        }
        return 0;
    }

private:
    std::shared_ptr<MappedFile> estimates_file_keep_alive;
    std::unordered_map<std::string_view, std::string_view> estimates;
};
}; // namespace catalyst
