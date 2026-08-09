// Copyright 2026 Siddharth Mohanty
// SPDX-License-Identifier: Apache-2.0

#include <fstream>

void create_dummy_file(const std::string &name) {
    std::ofstream f(name);
    f << "int main() {}";
    f.close();
}
