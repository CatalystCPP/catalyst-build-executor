import os
import random

ROOT_DIR = "benchmarks/heavy_repo"
SRC_DIR = os.path.join(ROOT_DIR, "src")
INCLUDE_DIR = os.path.join(ROOT_DIR, "include")
BUILD_DIR = os.path.join(ROOT_DIR, "build")

NUM_HEADERS = 1000
NUM_SOURCES = 1000
HEADERS_PER_HEADER = 3
HEADERS_PER_SOURCE = 10

def ensure_dirs():
    os.makedirs(SRC_DIR, exist_ok=True)
    os.makedirs(INCLUDE_DIR, exist_ok=True)
    # Build dir is usually created by the tool, but we might reference it

def generate_headers():
    for i in range(NUM_HEADERS):
        filename = os.path.join(INCLUDE_DIR, f"header_{i}.hpp")
        with open(filename, "w") as f:
            f.write(f"#pragma once\n\n")
            f.write(f"// Header {i}\n")

            # Include previous headers to ensure DAG (no cycles)
            # Pick 'HEADERS_PER_HEADER' random headers from 0 to i-1
            if i > 0:
                potential_deps = list(range(i))
                num_deps = min(len(potential_deps), HEADERS_PER_HEADER)
                deps = random.sample(potential_deps, num_deps)
                for dep in deps:
                    f.write(f"#include \"header_{dep}.hpp\"\n")

            f.write(f"\ninline int func_{i}() {{ return {i}; }}\n")

def generate_sources():
    source_files = []
    for i in range(NUM_SOURCES):
        name = f"source_{i}.cpp"
        filename = os.path.join(SRC_DIR, name)
        source_files.append(name)
        with open(filename, "w") as f:
            f.write(f"// Source {i}\n")

            # Include random headers
            deps = random.sample(range(NUM_HEADERS), min(NUM_HEADERS, HEADERS_PER_SOURCE))
            for dep in deps:
                f.write(f"#include \"header_{dep}.hpp\"\n")

            f.write(f"\nint source_func_{i}() {{ \n")
            f.write(f"    int sum = 0;\n")
            for dep in deps:
                f.write(f"    sum += func_{dep}();\n")
            f.write(f"    return sum;\n")
            f.write(f"}}\n")

    # Main file
    with open(os.path.join(SRC_DIR, "main.cpp"), "w") as f:
        f.write("#include <iostream>\n")
        for i in range(NUM_SOURCES):
             f.write(f"int source_func_{i}();\n")

        f.write("\nint main() {\n")
        f.write("    int total = 0;\n")
        for i in range(NUM_SOURCES):
            f.write(f"    total += source_func_{i}();\n")
        f.write("    std::cout << \"Total: \" << total << std::endl;\n")
        f.write("    return 0;\n")
        f.write("}\n")

    return source_files

def generate_manifest(source_files):
    manifest_path = os.path.join(ROOT_DIR, "catalyst.build")
    with open(manifest_path, "w") as f:
        f.write("DEF|cc|clang\n")
        f.write("DEF|cxx|clang++\n")
        f.write("DEF|cflags|\n")
        f.write("DEF|cxxflags|-std=c++20 -O0 -Iinclude\n") # O0 for speed of compilation during test
        f.write("DEF|ldflags|\n")
        f.write("DEF|ldlibs|\n")

        obj_files = []

        # Sources
        for src in source_files:
            obj_name = f"build/{src}.o"
            obj_files.append(obj_name)
            f.write(f"cxx|src/{src}|{obj_name}\n")

        # Main
        obj_files.append("build/main.cpp.o")
        f.write(f"cxx|src/main.cpp|build/main.cpp.o\n")

        # Link
        all_objs = ",".join(obj_files)
        f.write(f"ld|{all_objs}|build/heavy_app\n")

if __name__ == "__main__":
    ensure_dirs()
    print("Generating headers...")
    generate_headers()
    print("Generating sources...")
    srcs = generate_sources()
    print("Generating manifest...")
    generate_manifest(srcs)
    print("Done.")
