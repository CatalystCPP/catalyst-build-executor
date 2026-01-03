# CBE (Catalyst Build Executor)

CBE is a high-speed, low-level build orchestrator.
It is designed to be a machine-generated target for higher-level meta-build systems,
focusing on raw execution performance and precise dependency tracking.

## The `catalyst.build` Manifest

The manifest uses a strict pipe-delimited (`|`) format to ensure compatibility with Windows file paths and to prevent issues with editor-specific whitespace settings.

### 1. Global Definitions

Definitions configure the toolchain environment. These must appear at the top of the file.

| Variable | Description |
| --- | --- |
| `DEF\|cc\|<val>` | The C compiler (e.g., `/usr/bin/clang`). |
| `DEF\|cxx\|<val>` | The C++ compiler (e.g., `/usr/bin/clang++`). |
| `DEF\|cflags\|<val>` | Flags for `cc` steps. |
| `DEF\|cxxflags\|<val>` | Flags for `cxx` steps. |
| `DEF\|ldflags\|<val>` | Linker search paths and general flags. |
| `DEF\|ldlibs\|<val>` | Libraries to link against (e.g., `-lpthread`). |


> [!Note]
> You may override default command templates by defining
> `DEF|CCCompile|<cmd>`, `DEF|CXXCompile|<cmd>`, ` DEF|BINARYLINK|<cmd>`,
> `DEF|STATICLINK|<cmd>`, or `DEF|SHAREDLINK|<cmd>`.

---

### 2. Build Step Schema

Each build step is a single line following this format:

```text
<step_type>|<input_list>|<output_file>

```

* **`step_type`**: A mnemonic string representing the tool (see **Toolchain Mapping**).
* **`input_list`**: A comma-separated list of files (e.g., `src/main.cpp,src/util.cpp`).
* **Response Files**: For long input lists (common in link steps), use the `@path/to/file.rsp` syntax. CBE will expand
the contents of the `.rsp` file into the command line.
* **`output_file`**: The single file produced by this step. Append a `;` to the end of the line to mark this step as a **default target**.

---

### 3. Toolchain Mapping & Inferred Dependencies

CBE automatically manages dependency tracking (`.d` files). By default, it instructs the compiler to output dependency
information to `$out.d`.


| Key | Type | Default Command Template |
| --- | --- | --- |
| `cc` | C Compile | `$cc $cflags -MMD -MF $out.d -c $in -o $out` |
| `cxx` | C++ Compile | `$cxx $cxxflags -MMD -MF $out.d -c $in -o $out` |
| `ld` | Binary Link | `$cxx $in -o $out $ldflags $ldlibs` |
| `ar` | Static Link | `$ar rcs $out $in $ldflags $ldlibs` |
| `sld` | Shared Link | `$cxx -shared $in -o $out $ldflags` |


> **Dependency Logic**: CBE assumes that every `cc` and `cxx` step will produce a sidecar `.d` file at
> `<output_file>.d`. CBE reads these files after execution to populate its internal dependency graph
> for the next invocation.

---

## Example `catalyst.build`

```text
DEF|cc|clang
DEF|cxx|clang++
DEF|ccflags|
DEF|cxxflags|-std=c++20 -O3
DEF|ldflags|
DEF|ldlibs|

# Compile steps (depfiles inferred as build/main.o.d)
cxx|src/main.cpp|build/main.o
cxx|src/net.cpp|build/net.o

# Link step using a response file for many objects
ld|@build/inputs.rsp|build/app;
```
