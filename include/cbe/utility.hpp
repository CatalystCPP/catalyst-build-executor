#pragma once
#include <expected>
#include <string>

namespace catalyst {
template <typename T> using Result = std::expected<T, std::string>;
}
