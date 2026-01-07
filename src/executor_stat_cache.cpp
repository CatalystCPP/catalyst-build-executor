#include "cbe/executor.hpp"

#include <filesystem>
using namespace catalyst;

bool StatCache::Entry::operator<(const Entry &other) const {
    return path < other.path;
}
bool StatCache::Entry::operator<(const std::filesystem::path &other_path) const {
    return path < other_path;
}

auto StatCache::get_or_update(const std::filesystem::path &p)
    -> std::pair<std::filesystem::file_time_type, std::error_code> {
    auto is_path_less = [](const Entry &e, const std::filesystem::path &val) { return e.path < val; };
    {
        std::shared_lock read_lock(cache_mtx);

        auto it = std::lower_bound(cache.begin(), cache.end(), p, is_path_less);

        if (it != cache.end() && it->path == p) {
            return {it->time, it->ec};
        }
    }

    std::lock_guard write_lock(cache_mtx);
    auto it = std::lower_bound(cache.begin(), cache.end(), p, is_path_less);
    std::error_code ec;
    auto time = std::filesystem::last_write_time(p, ec);

    cache.insert(it, {p, time, ec}); // it is the element right below so we can add here safely
    return {time, ec};
}

bool StatCache::changed_since(const std::filesystem::path &input, std::filesystem::file_time_type output_time) {
    auto [input_time, ec] = get_or_update(input);
    if (ec)
        return true; // If input missing/error, assume rebuild needed
    return input_time >= output_time;
}
