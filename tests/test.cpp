// Copyright 2026 Siddharth Mohanty
// SPDX-License-Identifier: Apache-2.0

#include "cob-tests/test_suite.hpp"

#include <cassert>

int main(int argc, char **argv) {
    return !(stat_cache_test() && integration_test());
}
