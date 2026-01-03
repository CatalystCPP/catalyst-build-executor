#pragma once

#include <fcntl.h>
#include <filesystem>
#include <stdexcept>
#include <string_view>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace catalyst {

class MappedFile {
public:
    explicit MappedFile(const std::filesystem::path &path) {
        fd_ = open(path.c_str(), O_RDONLY);
        if (fd_ == -1) {
            throw std::runtime_error("Failed to open file: " + path.string());
        }

        struct stat sb;
        if (fstat(fd_, &sb) == -1) {
            close(fd_);
            throw std::runtime_error("Failed to stat file: " + path.string());
        }
        size_ = static_cast<size_t>(sb.st_size);

        if (size_ == 0) {
            data_ = nullptr;
            return;
        }

        void *addr = mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, fd_, 0);
        if (addr == MAP_FAILED) {
            close(fd_);
            throw std::runtime_error("Failed to mmap file: " + path.string());
        }
        data_ = static_cast<char *>(addr);
    }

    ~MappedFile() {
        if (data_) {
            munmap(data_, size_);
        }
        if (fd_ != -1) {
            close(fd_);
        }
    }

    std::string_view content() const {
        if (!data_)
            return {};
        return {data_, size_};
    }

    MappedFile(const MappedFile &) = delete;
    MappedFile &operator=(const MappedFile &) = delete;

private:
    int fd_ = -1;
    char *data_ = nullptr;
    size_t size_ = 0;
};

} // namespace catalyst
