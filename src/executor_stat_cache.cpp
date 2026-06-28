#include "cob/executor.hpp"

#include <algorithm>
#include <filesystem>
using catalyst::StatCache;

//NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)
bool StatCache::Entry::operator<(const Entry &other) const {
    return path < other.path;
}
bool StatCache::Entry::operator<(std::string_view other_path) const {
    return path < other_path;
}

auto StatCache::getOrUpdate(std::string_view p)
    -> std::pair<std::filesystem::file_time_type, std::error_code> {
    size_t idx = getBucketIndex(p);
    Bucket &b = buckets[idx];

    // 1. Shared (read) lock for the fast path (cached hits)
    {
        std::shared_lock<std::shared_mutex> read_lock(b.mtx);
        auto it = std::ranges::lower_bound(b.entries, p, {}, &Entry::path);
        if (it != b.entries.end() && it->path == p) {
            return {it->time, it->ec};
        }
    }

    // 2. Exclusive (write) lock for the fallback path (cache misses)
    std::lock_guard<std::shared_mutex> write_lock(b.mtx);
    auto it = std::ranges::lower_bound(b.entries, p, {}, &Entry::path);
    if (it != b.entries.end() && it->path == p) {
        return {it->time, it->ec};
    }

    std::error_code ec;
    std::filesystem::file_time_type time = std::filesystem::last_write_time(std::filesystem::path(p), ec);
    b.entries.insert(it, {.path=std::string(p), .time=time, .ec=ec});
    return {time, ec};
}

bool StatCache::changedSince(std::string_view input, std::filesystem::file_time_type output_time) {
    auto [input_time, ec] = getOrUpdate(input);
    if (ec)
        return true;
    return input_time >= output_time;
}

void StatCache::invalidate(std::string_view p) {
    size_t idx = getBucketIndex(p);
    Bucket &b = buckets[idx];

    std::lock_guard<std::shared_mutex> write_lock(b.mtx);
    auto it = std::ranges::lower_bound(b.entries, p, {}, &Entry::path);
    if (it != b.entries.end() && it->path == p) {
        b.entries.erase(it);
    }
}

size_t StatCache::getCacheSize() const {
    size_t total = 0;
    for (size_t i = 0; i < NUM_BUCKETS; ++i) {
        //NOLINTBEGIN(cppcoreguidelines-pro-type-const-cast)
        std::shared_lock<std::shared_mutex> lock(const_cast<std::shared_mutex&>(buckets[i].mtx));
        //NOLINTEND(cppcoreguidelines-pro-type-const-cast)
        total += buckets[i].entries.size();
    }
    return total;
}
//NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)
