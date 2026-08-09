// Copyright 2026 Siddharth Mohanty
// SPDX-License-Identifier: Apache-2.0

#include <memory>
#include <unistd.h>

namespace catalyst {
class FileDescriptor {
public:
    explicit FileDescriptor(int descriptor) : descriptor(descriptor) {
    }

    ~FileDescriptor() {
        if (descriptor >= 0) {
            ::close(descriptor);
        }
    }

    FileDescriptor(const FileDescriptor &) = delete;
    FileDescriptor &operator=(const FileDescriptor &) = delete;
    FileDescriptor(FileDescriptor &&) = delete;
    FileDescriptor &operator=(FileDescriptor &&) = delete;

    [[nodiscard]] int get() const {
        return descriptor;
    }

    bool close() {
        int descriptor_to_close = descriptor;
        descriptor = -1;
        return ::close(descriptor_to_close) == 0;
    }

private:
    int descriptor;
};
} // namespace catalyst
