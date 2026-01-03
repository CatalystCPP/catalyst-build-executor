#include "cbe/builder.hpp"

namespace catalyst {

Result<void> CBEBuilder::add_step(BuildStep &&bs) {
    auto res = graph_.add_step(std::move(bs));
    if (!res)
        return std::unexpected(res.error());
    return {};
}

} // namespace catalyst
