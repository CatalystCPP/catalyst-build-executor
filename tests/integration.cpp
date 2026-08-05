#include "cob-tests/test_suite.hpp"
#include "cob-tests/testing_utils.hpp"
#include "cob/binary.hpp"
#include "cob/builder.hpp"
#include "cob/executor.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <print>
#include <sstream>

using namespace catalyst;

bool rebuild_command_change_test() {
    std::println("Starting Command-Line/Flag Change Rebuild Test...");

    // Clean up any existing .catalyst.bin
    if (std::filesystem::exists(".catalyst.bin")) {
        std::filesystem::remove(".catalyst.bin");
    }

    create_dummy_file("catalyst.build");
    // Set catalyst.build mtime to 1 hour in the past to ensure output files are newer than it
    std::filesystem::last_write_time("catalyst.build",
                                     std::filesystem::last_write_time("catalyst.build") - std::chrono::hours(1));

    create_dummy_file("dummy_rebuild.c");

    // 1. Initial Build with -DTEST1
    {
        COBBuilder builder;
        builder.addDefinition("cc", "clang");
        builder.addDefinition("cflags", "-DTEST1");

        BuildStep step;
        step.tool = "cc";
        step.inputs = "dummy_rebuild.c";
        step.output = "dummy_rebuild.o";

        auto res = builder.addStep(std::move(step));
        if (!res) {
            std::println(std::cerr, "Failed to add step in run 1: {}", res.error());
            return false;
        }

        Executor executor(std::move(builder), ExecutorConfig{});
        auto exec_res = executor.execute();
        if (!exec_res) {
            std::println(std::cerr, "Run 1 execution failed: {}", exec_res.error());
            return false;
        }
    }

    // 2. Second Build with SAME flags (-DTEST1). Should skip!
    {
        COBBuilder builder;
        std::cout << "[Run 2] parsing bin..." << std::endl;
        auto parse_res = parseBin(builder);
        if (!parse_res) {
            std::println(std::cerr, "Failed to parse .catalyst.bin in run 2: {}", parse_res.error());
            return false;
        }

        builder.addDefinition("cflags", "-DTEST1");

        std::cout << "[Run 2] creating executor..." << std::endl;
        Executor executor(std::move(builder), ExecutorConfig{});
        std::cout << "[Run 2] executing..." << std::endl;
        auto exec_res = executor.execute();
        if (!exec_res) {
            std::println(std::cerr, "Run 2 execution failed: {}", exec_res.error());
            return false;
        }
        std::cout << "[Run 2] execution finished" << std::endl;
    }
    std::cout << "[Run 2] block exited" << std::endl;

    // 3. Third Build with DIFFERENT flags (-DTEST2). Should rebuild!
    {
        COBBuilder builder;
        std::cout << "[Run 3] parsing bin..." << std::endl;
        auto parse_res = parseBin(builder);
        if (!parse_res) {
            std::println(std::cerr, "Failed to parse .catalyst.bin in run 3: {}", parse_res.error());
            return false;
        }

        builder.addDefinition("cflags", "-DTEST2");

        std::cout << "[Run 3] creating executor..." << std::endl;
        Executor executor(std::move(builder), ExecutorConfig{});
        std::cout << "[Run 3] executing..." << std::endl;
        auto exec_res = executor.execute();
        if (!exec_res) {
            std::println(std::cerr, "Run 3 execution failed: {}", exec_res.error());
            return false;
        }
        std::cout << "[Run 3] execution finished" << std::endl;
    }

    // Cleanup
    if (std::filesystem::exists("dummy_rebuild.c"))
        std::filesystem::remove("dummy_rebuild.c");
    if (std::filesystem::exists("dummy_rebuild.o"))
        std::filesystem::remove("dummy_rebuild.o");
    if (std::filesystem::exists("dummy_rebuild.o.d"))
        std::filesystem::remove("dummy_rebuild.o.d");
    if (std::filesystem::exists("catalyst.build"))
        std::filesystem::remove("catalyst.build");
    if (std::filesystem::exists(".catalyst.bin"))
        std::filesystem::remove(".catalyst.bin");

    std::println("Command-Line/Flag Change Rebuild Test passed!");
    return true;
}

bool binaryChecksumTest() {
    std::println("Starting Binary Checksum Test...");

    COBBuilder builder;
    builder.addDefinition("cc", "clang");
    if (auto emit_result = emitBin(builder); !emit_result) {
        std::println(std::cerr, "Failed to emit binary cache: {}", emit_result.error());
        return false;
    }

    std::fstream binary(".catalyst.bin", std::ios::in | std::ios::out | std::ios::binary);
    binary.seekg(-1, std::ios::end);
    char byte = 0;
    binary.read(&byte, 1);
    byte ^= 1;
    binary.seekp(-1, std::ios::end);
    binary.write(&byte, 1);
    binary.close();
    if (!binary) {
        std::println(std::cerr, "Failed to corrupt binary cache for checksum test");
        std::filesystem::remove(".catalyst.bin");
        return false;
    }

    COBBuilder parsed_builder;
    auto parse_result = parseBin(parsed_builder);
    std::filesystem::remove(".catalyst.bin");
    if (parse_result || !parse_result.error().contains("Checksum mismatch")) {
        std::println(std::cerr, "Corrupted binary cache was not rejected by its checksum");
        return false;
    }

    std::println("Binary Checksum Test passed!");
    return true;
}

bool binaryEndiannessTest() {
    std::println("Starting Binary Endianness Test...");

    COBBuilder builder;
    builder.addDefinition("cc", "clang");
    if (auto emit_result = emitBin(builder); !emit_result) {
        std::println(std::cerr, "Failed to emit binary cache: {}", emit_result.error());
        return false;
    }

    constexpr std::streamoff MAGIC_SIZE = 8;
    std::fstream binary(".catalyst.bin", std::ios::in | std::ios::out | std::ios::binary);
    binary.seekg(MAGIC_SIZE);
    std::array<char, sizeof(uint64_t)> marker{};
    binary.read(marker.data(), marker.size());
    std::ranges::reverse(marker);
    binary.seekp(MAGIC_SIZE);
    binary.write(marker.data(), marker.size());
    binary.close();
    if (!binary) {
        std::println(std::cerr, "Failed to reverse binary cache endianness marker");
        std::filesystem::remove(".catalyst.bin");
        return false;
    }

    COBBuilder parsed_builder;
    auto parse_result = parseBin(parsed_builder);
    std::filesystem::remove(".catalyst.bin");
    if (parse_result || !parse_result.error().contains("Incompatible endianness")) {
        std::println(std::cerr, "Opposite-endian binary cache was not rejected");
        return false;
    }

    std::println("Binary Endianness Test passed!");
    return true;
}

bool build_step_extra_test() {
    std::println("Starting BuildStepExtra Spacing and Compilation Test...");

    // Test cases for invalid spacing
    const std::vector<std::string> invalid_manifests = {
        "cc|dummy_extra.c|dummy_extra.o|extra=-DEXTRA_TEST_FLAG",
        "cc|dummy_extra.c|dummy_extra.o|extra =-DEXTRA_TEST_FLAG",
        "cc|dummy_extra.c|dummy_extra.o|extra= -DEXTRA_TEST_FLAG",
        "cc|dummy_extra.c|dummy_extra.o|invalid_key = -DEXTRA_TEST_FLAG",
        "cc|dummy_extra.c|dummy_extra.o|extra"};

    for (const auto &manifest_content : invalid_manifests) {
        std::ofstream out("catalyst.build");
        out << manifest_content << "\n";
        out.close();

        COBBuilder builder;
        auto res = parse(builder, "catalyst.build");
        std::filesystem::remove("catalyst.build");
        if (res) {
            std::println(
                std::cerr, "Failed: Manifest parsed successfully but should have failed: {}", manifest_content);
            return false;
        }
    }

    // Test case for valid spacing and actual compilation success
    std::ofstream src("dummy_extra.c");
    src << "#ifndef EXTRA_TEST_FLAG\n#error \"EXTRA_TEST_FLAG not defined\"\n#endif\n";
    src.close();

    const std::vector<std::string> valid_manifests = {
        "DEF|cc|clang\ncc|dummy_extra.c|dummy_extra.o|extra = -DEXTRA_TEST_FLAG",
        "DEF|cc|clang\ncc|dummy_extra.c|dummy_extra.o|extra  =  -DEXTRA_TEST_FLAG",
        "DEF|cc|clang\ncc|dummy_extra.c|dummy_extra.o|extra \t=\t -DEXTRA_TEST_FLAG"};

    for (const auto &manifest_content : valid_manifests) {
        if (std::filesystem::exists("dummy_extra.o")) {
            std::filesystem::remove("dummy_extra.o");
        }
        if (std::filesystem::exists("dummy_extra.o.d")) {
            std::filesystem::remove("dummy_extra.o.d");
        }

        std::ofstream out("catalyst.build");
        out << manifest_content << "\n";
        out.close();

        COBBuilder builder;
        auto res = parse(builder, "catalyst.build");
        if (!res) {
            std::println(std::cerr, "Failed to parse valid manifest: {}", res.error());
            std::filesystem::remove("catalyst.build");
            std::filesystem::remove("dummy_extra.c");
            return false;
        }

        Executor executor(std::move(builder), ExecutorConfig{});
        auto exec_res = executor.execute();
        std::filesystem::remove("catalyst.build");
        if (!exec_res) {
            std::println(std::cerr, "Execution failed for valid manifest: {}", exec_res.error());
            std::filesystem::remove("dummy_extra.c");
            return false;
        }
    }

    // Cleanup
    if (std::filesystem::exists("dummy_extra.c"))
        std::filesystem::remove("dummy_extra.c");
    if (std::filesystem::exists("dummy_extra.o"))
        std::filesystem::remove("dummy_extra.o");
    if (std::filesystem::exists("dummy_extra.o.d"))
        std::filesystem::remove("dummy_extra.o.d");

    std::println("BuildStepExtra Spacing and Compilation Test passed!");
    return true;
}

bool sharedLinkFlagsTest() {
    std::println("Starting Shared-Link Flags Test...");

    COBBuilder builder;
    builder.addDefinition("linker", "clang++");
    builder.addDefinition("ldflags", "-Wl,--no-undefined -L/custom/lib");
    builder.addDefinition("ldlibs", "-lcustom");

    BuildStep step;
    step.tool = "sld";
    step.inputs = "dummy.o";
    step.output = "libdummy.so";

    if (auto result = builder.addStep(std::move(step)); !result) {
        std::println(std::cerr, "Failed to add shared-link step: {}", result.error());
        return false;
    }

    Executor executor(std::move(builder), ExecutorConfig{});
    std::ostringstream commands;
    std::streambuf *original_buffer = std::cout.rdbuf(commands.rdbuf());
    auto result = executor.emitCommands();
    std::cout.rdbuf(original_buffer);

    if (!result) {
        std::println(std::cerr, "Failed to emit shared-link command: {}", result.error());
        return false;
    }

    const std::string command = commands.str();
    if (!command.contains("-Wl,--no-undefined") || !command.contains("-L/custom/lib") ||
        !command.contains("-lcustom")) {
        std::println(std::cerr, "Shared-link command omitted linker flags or libraries: {}", command);
        return false;
    }

    std::println("Shared-Link Flags Test passed!");
    return true;
}

bool integration_test() {
    // Setup
    std::println("Starting Integration Test...");
    create_dummy_file("catalyst.build");
    std::filesystem::last_write_time("catalyst.build",
                                     std::filesystem::last_write_time("catalyst.build") - std::chrono::hours(1));

    create_dummy_file("dummy.c");

    COBBuilder builder;
    builder.addDefinition("cc", "clang"); // Mock cc with echo
    builder.addDefinition("cflags", "-DTEST");

    BuildStep step;
    step.tool = "cc";
    step.inputs = "dummy.c";
    step.output = "dummy.o";

    // Add step
    auto res = builder.addStep(std::move(step));
    if (!res) {
        std::println(std::cerr, "Failed to add step: {}", res.error());
        return false;
    }

    Executor executor(std::move(builder), ExecutorConfig{});
    auto exec_res = executor.execute();

    if (!exec_res) {
        std::println(std::cerr, "Execution failed: {}", exec_res.error());
        return false;
    }

    std::println("Test passed!");

    // Cleanup
    if (std::filesystem::exists("dummy.c"))
        std::filesystem::remove("dummy.c");
    if (std::filesystem::exists("dummy.o"))
        std::filesystem::remove("dummy.o");
    if (std::filesystem::exists("dummy.o.d"))
        std::filesystem::remove("dummy.o.d");
    if (std::filesystem::exists("catalyst.build"))
        std::filesystem::remove("catalyst.build");

    // Run binary checksum test
    if (!binaryChecksumTest()) {
        return false;
    }

    // Run binary endianness test
    if (!binaryEndiannessTest()) {
        return false;
    }

    // Run BuildStepExtra spacing and compilation test
    if (!build_step_extra_test()) {
        return false;
    }

    // Run command line/flag change rebuild test
    if (!rebuild_command_change_test()) {
        return false;
    }

    return sharedLinkFlagsTest();
}
