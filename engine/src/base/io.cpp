#include "engine/base/io.h"

#include <windows.h>

using namespace engine;

struct File : Io {
    File(std::string_view name, Mode mode) : Io(name) {
        DWORD dwAccess = ((mode & eRead) ? GENERIC_READ : 0)
                       | ((mode & eWrite) ? GENERIC_WRITE : 0);
        DWORD dwShare = (mode & eWrite ? 0 : FILE_SHARE_READ);
        DWORD dwCreate = (mode & eWrite ? CREATE_NEW : OPEN_EXISTING);
        handle = CreateFileA(std::string(name).c_str(), dwAccess, dwShare, nullptr, dwCreate, FILE_ATTRIBUTE_NORMAL, nullptr);
    }
    
    File(File&& other) : Io(other.name) {
        handle = other.handle;
        other.handle = nullptr;
    }

    ~File() override {
        if (handle != nullptr) {
            CloseHandle(handle);
        }
    }

    size_t read(void *dst, size_t size) override {
        DWORD total = 0;
        ReadFile(handle, dst, static_cast<DWORD>(size), &total, nullptr);
        return total;
    }

    size_t write(const void *src, size_t size) override {
        DWORD total = 0;
        WriteFile(handle, src, static_cast<DWORD>(size), &total, nullptr);
        return total;
    }

private:
    HANDLE handle = nullptr;
};

Io::Io(std::string_view name)
    : name(name)
{ }

void Io::write(std::string_view text) {
    write(text.data(), text.size());
}

Io *Io::open(std::string_view name, Mode mode) {
    return new File(name, mode);
}
