#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#include <exception>
#endif

#include "process_exec_linux.hpp"

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <iterator>
#include <ranges>
#include <spawn.h>
#include <string>
#include <string_view>
#include <sys/wait.h>
#include <system_error>
#include <unistd.h>
#include <utility>
#include <vector>

namespace catalyst {
namespace {

constexpr size_t BUFFER_SIZE = 1U << 14; // 16 KiB
class FileDescriptor {
public:
    FileDescriptor() = default;
    explicit FileDescriptor(int value) noexcept : value(value) {
    }

    FileDescriptor(const FileDescriptor &) = delete;
    FileDescriptor &operator=(const FileDescriptor &) = delete;

    FileDescriptor(FileDescriptor &&other) noexcept : value(std::exchange(other.value, -1)) {
    }
    FileDescriptor &operator=(FileDescriptor &&other) noexcept {
        if (this != &other) {
            reset();
            value = std::exchange(other.value, -1);
        }
        return *this;
    }

    ~FileDescriptor() {
        reset();
    }

    [[nodiscard]] int get() const noexcept {
        return value;
    }

    void reset(int new_value = -1) noexcept {
        if (value >= 0) {
            ::close(value);
        }
        value = new_value;
    }

private:
    int value = -1;
};

class SpawnFileActions {
public:
    SpawnFileActions() : error(::posix_spawn_file_actions_init(&actions)) {
    }
    SpawnFileActions(const SpawnFileActions &) = delete;
    SpawnFileActions &operator=(const SpawnFileActions &) = delete;
    SpawnFileActions(SpawnFileActions &&) = delete;
    SpawnFileActions &operator=(SpawnFileActions &&) = delete;
    ~SpawnFileActions() {
        if (error == 0) {
            ::posix_spawn_file_actions_destroy(&actions);
        }
    }

    [[nodiscard]] int getError() const noexcept {
        return error;
    }
    [[nodiscard]] posix_spawn_file_actions_t *get() noexcept {
        return &actions;
    }

private:
    posix_spawn_file_actions_t actions{};
    int error = 0;
};

class SpawnAttributes {
public:
    SpawnAttributes() : error(::posix_spawnattr_init(&attributes)) {
    }
    SpawnAttributes(SpawnAttributes &&) = delete;
    SpawnAttributes &operator=(SpawnAttributes &&) = delete;
    SpawnAttributes(const SpawnAttributes &) = delete;
    SpawnAttributes &operator=(const SpawnAttributes &) = delete;
    ~SpawnAttributes() {
        if (error == 0) {
            ::posix_spawnattr_destroy(&attributes);
        }
    }

    [[nodiscard]] int getError() const noexcept {
        return error;
    }
    [[nodiscard]] posix_spawnattr_t *get() noexcept {
        return &attributes;
    }

private:
    posix_spawnattr_t attributes{};
    int error = 0;
};

[[nodiscard]] bool environmentEntryHasKey(std::string_view entry, std::string_view key) {
    return entry.size() > key.size() && entry[key.size()] == '=' && entry.starts_with(key);
}

[[nodiscard]] bool isOverridden(std::string_view entry,
                                const std::vector<std::pair<std::string, std::string>> &overrides) {
    return std::ranges::any_of(overrides,
                               [&entry](const auto &pair) { return environmentEntryHasKey(entry, pair.first); });
}

struct Environment {
    std::vector<std::string> strings;
    std::vector<char *> pointers;
};

[[nodiscard]] std::string formatSystemError(std::string_view operation, int error) {
    return std::format("{}: {}", operation, std::error_code(error, std::generic_category()).message());
}

[[nodiscard]] Environment buildEnvironment(const std::vector<std::pair<std::string, std::string>> &overrides) {
    Environment result;
    const auto environ_dist = []() {
        size_t count = 0;
        for (char **entry = ::environ; entry && *entry; ++entry) {
            ++count;
        }
        return count;
    }();
    result.strings.reserve(environ_dist + overrides.size());

    for (char **entry = ::environ; entry != nullptr && *entry != nullptr; ++entry) {
        if (!isOverridden(*entry, overrides)) {
            result.strings.emplace_back(*entry);
        }
    }

    std::ranges::transform(
        overrides, std::back_inserter(result.strings), [](const auto &environment_override) -> std::string {
            const auto &[key, value] = environment_override;
            return std::format("{}={}", key, value);
        });

    result.pointers.reserve(result.strings.size() + 1U);
    std::ranges::transform(
        result.strings, std::back_inserter(result.pointers), [](std::string &entry) -> char * { return entry.data(); });
    result.pointers.push_back(nullptr);
    return result;
}

[[nodiscard]] std::vector<char *> buildArgv(const std::vector<std::string> &args) {
    // NOLINTBEGIN(cppcoreguidelines-pro-type-const-cast)
    std::vector<char *> result =
        args | std::views::transform([](const std::string &arg) { return const_cast<char *>(arg.c_str()); }) |
        std::ranges::to<std::vector<char *>>();
    // NOLINTEND(cppcoreguidelines-pro-type-const-cast)
    result.push_back(nullptr);
    return result;
}

[[nodiscard]] int configureAttributes(SpawnAttributes &attributes) {
    if (attributes.getError() != 0) {
        return attributes.getError();
    }

    sigset_t empty_mask{};
    if (::sigemptyset(&empty_mask) < 0) {
        return errno;
    }

    sigset_t default_signals{};
    if (::sigfillset(&default_signals) < 0) {
        return errno;
    }

    int error = ::posix_spawnattr_setsigmask(attributes.get(), &empty_mask);
    if (error != 0) {
        return error;
    }
    error = ::posix_spawnattr_setsigdefault(attributes.get(), &default_signals);
    if (error != 0) {
        return error;
    }
    return ::posix_spawnattr_setflags(attributes.get(), POSIX_SPAWN_SETSIGDEF | POSIX_SPAWN_SETSIGMASK);
}

[[nodiscard]] int configureFileActions(SpawnFileActions &actions,
                                       const std::optional<std::string> &working_dir,
                                       int capture_read,
                                       int capture_write) {
    if (actions.getError() != 0) {
        return actions.getError();
    }

    int error = 0;
    if (working_dir) {
        error = ::posix_spawn_file_actions_addchdir_np(actions.get(), working_dir->c_str());
        if (error != 0) {
            return error;
        }
    }

    if (capture_write >= 0) {
        error = ::posix_spawn_file_actions_adddup2(actions.get(), capture_write, STDOUT_FILENO);
        if (error != 0) {
            return error;
        }
        error = ::posix_spawn_file_actions_adddup2(actions.get(), capture_write, STDERR_FILENO);
        if (error != 0) {
            return error;
        }
        error = ::posix_spawn_file_actions_addclose(actions.get(), capture_read);
        if (error != 0) {
            return error;
        }
        error = ::posix_spawn_file_actions_addclose(actions.get(), capture_write);
        if (error != 0) {
            return error;
        }
    }

    return ::posix_spawn_file_actions_addclosefrom_np(actions.get(), STDERR_FILENO + 1);
}

[[nodiscard]] Result<int> waitForProcess(pid_t process) {
    int status = 0;
    while (::waitpid(process, &status, 0) < 0) {
        if (errno != EINTR) {
            return std::unexpected(formatSystemError("Failed to wait for process", errno));
        }
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }
    return std::unexpected("Process stopped without exiting or being terminated by a signal");
}

[[nodiscard]] Result<void> drainOutput(int fd, std::string &output) {
    std::array<std::uint8_t, BUFFER_SIZE> buffer2; // NOLINT(cppcoreguidelines-pro-type-member-init)
    while (true) {
        const ssize_t bytes_read = ::read(fd, buffer2.data(), buffer2.size());
        if (bytes_read > 0) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            output.append(reinterpret_cast<const char *>(buffer2.data()), static_cast<std::size_t>(bytes_read));
            continue;
        }
        if (bytes_read == 0) {
            return {};
        }
        if (errno != EINTR) {
            return std::unexpected(formatSystemError("Failed to read captured process output", errno));
        }
    }
}
} // namespace

Result<std::pair<int, std::string>>
processExecLinux(const std::vector<std::string> &args,
                 std::optional<std::string> working_dir,
                 std::optional<std::vector<std::pair<std::string, std::string>>> env,
                 bool capture_output) {
    if (args.empty()) {
        return std::unexpected("Cannot execute empty command");
    }

    FileDescriptor capture_read;
    FileDescriptor capture_write;
    if (capture_output) {
        std::array<int, 2> pipe_fds = {-1, -1};
        if (::pipe2(pipe_fds.data(), O_CLOEXEC) < 0) {
            return std::unexpected(formatSystemError("Failed to create pipe for capturing output", errno));
        }
        capture_read.reset(pipe_fds[0]);
        capture_write.reset(pipe_fds[1]);
    }

    SpawnFileActions actions;
    if (int error = configureFileActions(actions, working_dir, capture_read.get(), capture_write.get()); error != 0) {
        return std::unexpected(formatSystemError("Failed to configure file actions for posix_spawn", error));
    }

    SpawnAttributes attributes;

    if (int error = configureAttributes(attributes); error != 0) {
        return std::unexpected(formatSystemError("Failed to configure attributes for posix_spawn", error));
    }

    std::vector<char *> argv = buildArgv(args);
    Environment child_environment;
    char **environment = ::environ;
    if (env) {
        child_environment = buildEnvironment(*env);
        environment = child_environment.pointers.data();
    }

    pid_t process = -1;

    if (int error = ::posix_spawnp(&process, argv.front(), actions.get(), attributes.get(), argv.data(), environment);
        error != 0) {
        return std::unexpected(formatSystemError(std::format("Failed to spawn process '{}'", args.front()), error));
    }

    capture_write.reset();
    std::string captured;
    Result<void> read_result;
    if (capture_output) {
        read_result = drainOutput(capture_read.get(), captured);
        capture_read.reset();
    }

    Result<int> status = waitForProcess(process);
    if (!read_result) {
        return std::unexpected(std::move(read_result.error()));
    }
    if (!status) {
        return std::unexpected(std::move(status.error()));
    }
    return std::pair<int, std::string>{*status, std::move(captured)};
}
} // namespace catalyst
