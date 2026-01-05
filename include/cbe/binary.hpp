#pragma once

#include "cbe/builder.hpp"
#include "cbe/utility.hpp"

namespace catalyst {

Result<void> parse_bin(CBEBuilder &builder);
Result<void> emit_bin(CBEBuilder &builder);

} // namespace catalyst
