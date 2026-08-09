// Copyright 2026 Siddharth Mohanty
// SPDX-License-Identifier: Apache-2.0

#include "cob/parser.hpp"

#include "cob/binary.hpp"
#include "cob/builder.hpp"
#include "cob/file_handle.hpp"
#include "cob/utility.hpp"

#include <iostream>
#include <memory>
#include <print>
#include <string_view>

namespace catalyst {

namespace {

Result<void> parseDEF(const std::string_view line, COBBuilder &builder) {
    size_t first_pipe = line.find('|');
    if (first_pipe == std::string_view::npos) {
        return std::unexpected(std::format("Malformed def line (missing first pipe): {}", line));
    }

    size_t second_pipe = line.find('|', first_pipe + 1);
    if (second_pipe == std::string_view::npos) {
        return std::unexpected(std::format("Malformed def line (missing second pipe): {}", line));
    }

    builder.addDefinition(line.substr(first_pipe + 1, second_pipe - (first_pipe + 1)), // key
                          line.substr(second_pipe + 1)                                 // value
    );

    return {};
}

Result<void> parseStep(const std::string_view line, COBBuilder &builder) {
    size_t first_pipe = line.find('|');
    if (first_pipe == std::string_view::npos) {
        return std::unexpected(std::format("Malformed step line (missing first pipe): {}", line));
    }

    size_t second_pipe = line.find('|', first_pipe + 1);
    if (second_pipe == std::string_view::npos) {
        return std::unexpected(std::format("Malformed step line (missing second pipe): {}", line));
    }

    std::string_view tool = line.substr(0, first_pipe);
    std::string_view inputs = line.substr(first_pipe + 1, second_pipe - (first_pipe + 1));
    std::string_view output;
    std::string_view extra_flags;

    size_t third_pipe = line.find('|', second_pipe + 1);
    if (third_pipe == std::string_view::npos) {
        output = line.substr(second_pipe + 1);
    } else {
        output = line.substr(second_pipe + 1, third_pipe - (second_pipe + 1));
        std::string_view extra_part = line.substr(third_pipe + 1);
        if (!extra_part.starts_with("extra")) {
            return std::unexpected(std::format("Malformed step extra part (must start with 'extra'): {}", line));
        }
        std::string_view after_extra = extra_part.substr(sizeof("extra") - 1);
        if (after_extra.empty() || (after_extra[0] != ' ' && after_extra[0] != '\t')) {
            return std::unexpected(std::format("Malformed step extra part (missing spacing before '='): {}", line));
        }
        size_t eq_pos = after_extra.find('=');
        if (eq_pos == std::string_view::npos) {
            return std::unexpected(std::format("Malformed step extra part (missing '='): {}", line));
        }
        std::string_view before_eq = after_extra.substr(0, eq_pos);
        for (char c : before_eq) {
            if (c != ' ' && c != '\t') {
                return std::unexpected(
                    std::format("Malformed step extra part (invalid character before '='): {}", line));
            }
        }
        std::string_view after_eq = after_extra.substr(eq_pos + 1);
        if (after_eq.empty() || (after_eq[0] != ' ' && after_eq[0] != '\t')) {
            return std::unexpected(std::format("Malformed step extra part (missing spacing after '='): {}", line));
        }
        size_t val_start = after_eq.find_first_not_of(" \t");
        if (val_start != std::string_view::npos) {
            extra_flags = after_eq.substr(val_start);
        }
    }

    Result<void> res = builder.addStep({.tool = tool,
                                        .inputs = inputs,
                                        .output = output,
                                        .opaque_inputs = {},
                                        .depfile_inputs = {},
                                        .parsed_inputs = {},
                                        .extra_flags = extra_flags});
    if (!res) {
        return std::unexpected(res.error());
    }
    return {};
}

} // namespace

Result<void> parse(COBBuilder &builder, const std::filesystem::path &path) {
#if FF_cob__binary == 1
    if (std::filesystem::exists(".catalyst.bin") &&
        std::filesystem::last_write_time(".catalyst.bin") > std::filesystem::last_write_time(path)) {
        if (auto res = parseBin(builder); !res) {
            std::println(std::cerr, "Failed to parse binary (.catalyst.bin): {}", res.error());
            std::println("Falling back to configuration.", res.error());
        } else {
            return {};
        }
    }
#endif
    std::string_view content;
    try {
        auto file = std::make_shared<MappedFile>(path);
        builder.addResource(file);
        content = file->content();
    } catch (const std::exception &err) {
        return std::unexpected(err.what());
    }

    size_t start = 0;
    while (start < content.size()) {
        size_t end = content.find('\n', start);
        // last line of the file
        if (end == std::string_view::npos) {
            end = content.size();
        }

        std::string_view line = content.substr(start, end - start);
        // windows CRLF handling
        if (!line.empty() && line.back() == '\r') {
            line.remove_suffix(1);
        }

        if (!line.empty()) {
            if (line.starts_with("#")) {
                // Comment
            } else if (line.starts_with("DEF|")) {
                auto res = parseDEF(line, builder);
                if (!res)
                    return res;
            } else [[likely]] {
                auto res = parseStep(line, builder);
                if (!res)
                    return res;
            }
        }

        start = end + 1;
    }
#if FF_cob__binary
    auto _ = emitBin(builder);
#endif
    return {};
}

} // namespace catalyst
