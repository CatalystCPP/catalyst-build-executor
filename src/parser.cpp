#include "cbe/parser.hpp"

#include "cbe/builder.hpp"
#include "cbe/utility.hpp"
#include "mmap.hpp" // Use our internal mmap header

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace catalyst {

namespace {

Result<void> parse_def(const std::string_view line, CBEBuilder &builder) {
    size_t first_pipe = line.find('|');
    if (first_pipe == std::string_view::npos) {
        return std::unexpected(std::format("Malformed def line (missing first pipe): {}", line));
    }

    size_t second_pipe = line.find('|', first_pipe + 1);
    if (second_pipe == std::string_view::npos) {
        return std::unexpected(std::format("Malformed def line (missing second pipe): {}", line));
    }

    builder.add_definition(line.substr(first_pipe + 1, second_pipe - (first_pipe + 1)), // key
                           line.substr(second_pipe + 1)                                 // value
    );

    return {};
}

Result<void> parse_step(const std::string_view line, CBEBuilder &builder) {
    size_t first_pipe = line.find('|');
    if (first_pipe == std::string_view::npos) {
        return std::unexpected(std::format("Malformed step line (missing first pipe): {}", line));
    }

    size_t second_pipe = line.find('|', first_pipe + 1);
    if (second_pipe == std::string_view::npos) {
        return std::unexpected(std::format("Malformed step line (missing second pipe): {}", line));
    }

    builder.add_step({
        line.substr(0, first_pipe),                                  // step_type
        line.substr(first_pipe + 1, second_pipe - (first_pipe + 1)), // inputs
        line.substr(second_pipe + 1)                                 // output
    });
    return {};
}

} // namespace

Result<void> parse(CBEBuilder &builder, const std::filesystem::path &path) {
    std::string_view content;
    try {
        auto file = std::make_shared<MappedFile>(path);
        builder.add_resource(file);
        content = file->content();
    } catch (const std::exception &err) {
        return std::unexpected(err.what());
    }

    size_t start = 0;
    while (start < content.size()) {
        size_t end = content.find('\n', start);
        if (end == std::string_view::npos) {
            end = content.size();
        }

        std::string_view line = content.substr(start, end - start);
        if (!line.empty() && line.back() == '\r') {
            line.remove_suffix(1);
        }

        if (!line.empty()) {
            char first = line[0];

            if (first == '#') {
                // Comment
            } else if (line.starts_with("DEF|")) {
                auto res = parse_def(line, builder);
                if (!res)
                    return res;
            } else {
                auto res = parse_step(line, builder);
                if (!res)
                    return res;
            }
        }

        start = end + 1;
    }
    return {};
}

} // namespace catalyst
