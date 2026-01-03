#pragma once

#include "cbe/builder.hpp"
#include "cbe/utility.hpp"

#include <filesystem>

namespace catalyst {

Result<void> parse(CBEBuilder &builder, const std::filesystem::path &path);

} // namespace catalyst
