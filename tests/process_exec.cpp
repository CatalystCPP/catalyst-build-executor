// Copyright 2026 Siddharth Mohanty
// SPDX-License-Identifier: Apache-2.0

#include "cob-tests/test_suite.hpp"
#include "cob/process_exec.hpp"

#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

bool process_exec_test() {
    using Environment = std::vector<std::pair<std::string, std::string>>;
    using namespace std::string_literals;

    for (const auto &key : {""s, "BAD=KEY"s, "BAD\0KEY"s}) {
        auto result = catalyst::process_exec(
            {"/bin/sh", "-c", "exit 0"}, std::nullopt, Environment{{key, "value"}}, true);
        if (result || !result.error().starts_with("Invalid environment variable key:")) {
            std::cerr << "Invalid environment key was not rejected\n";
            return false;
        }
    }

    auto invalid_value = catalyst::process_exec(
        {"/bin/sh", "-c", "exit 0"}, std::nullopt, Environment{{"COB_TEST_SECRET", "secret\0suffix"s}}, true);
    if (invalid_value || invalid_value.error() != "Invalid environment variable value for key 'COB_TEST_SECRET'") {
        std::cerr << "Invalid environment value was not rejected with a redacted error\n";
        return false;
    }

    auto valid = catalyst::process_exec(
        {"/bin/sh", "-c", "printf '%s:%s' \"$COB_TEST_VALUE\" \"$COB_TEST_EMPTY\""},
        std::nullopt,
        Environment{{"COB_TEST_VALUE", "a=b"}, {"COB_TEST_EMPTY", ""}},
        true);
    if (!valid || valid->first != 0 || valid->second != "a=b:") {
        std::cerr << "Valid environment values did not reach the child process\n";
        return false;
    }
    return true;
}
