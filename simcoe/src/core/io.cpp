#include "simcoe/core/io.h"
#include "simcoe/core/panic.h"
#include "simcoe/core/win32.h"

using namespace simcoe;

struct File final : Io {
    File(std::string_view name, Mode mode) : Io(name, mode) {
        DWORD dwAccess = ((mode & eRead) ? GENERIC_READ : 0)
                       | ((mode & eWrite) ? GENERIC_WRITE : 0);
        DWORD dwShare = (mode & eWrite ? 0 : FILE_SHARE_READ);
        DWORD dwCreate = (mode & eWrite ? CREATE_ALWAYS : OPEN_EXISTING);
        handle = CreateFileA(std::string(name).c_str(), dwAccess, dwShare, nullptr, dwCreate, FILE_ATTRIBUTE_NORMAL, nullptr);
    }

    ~File() override {
        CloseHandle(handle);
    }

    size_t innerRead(void *dst, size_t size) override {
        DWORD total = 0;
        ReadFile(handle, dst, static_cast<DWORD>(size), &total, nullptr);
        return total;
    }

    size_t innerWrite(const void *src, size_t size) override {
        DWORD total = 0;
        WriteFile(handle, src, static_cast<DWORD>(size), &total, nullptr);
        return total;
    }

    size_t size() override {
        LARGE_INTEGER size;
        GetFileSizeEx(handle, &size);
        return size_t(size.QuadPart);
    }

    bool valid() const override {
        return handle != INVALID_HANDLE_VALUE;
    }

private:
    HANDLE handle = nullptr;
};


size_t Io::read(void *dst, size_t size) {
    ASSERT(mode & Io::eRead);
    return innerRead(dst, size);
}

size_t Io::write(const void *src, size_t size) {
    ASSERT(mode & Io::eWrite);
    return innerWrite(src, size);
}

void Io::write(std::string_view text) {
    write(text.data(), text.size());
}

Io *Io::open(std::string_view name, Mode mode) {
    return new File(name, mode);
}
