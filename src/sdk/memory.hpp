#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <cstdint>
#include <string>

class Memory {
public:
    Memory() = default;
    ~Memory() { close(); }

    Memory(const Memory&) = delete;
    Memory& operator=(const Memory&) = delete;

    bool attach(const wchar_t* process_name);
    void close();
    bool ok() const { return handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE; }
    DWORD pid() const { return pid_; }

    uintptr_t module_base(const wchar_t* module_name) const;

    template <typename T>
    T read(uintptr_t address) const
    {
        T value{};
        if (!address || !ok())
            return value;
        SIZE_T read = 0;
        ReadProcessMemory(handle_, reinterpret_cast<LPCVOID>(address), &value, sizeof(T), &read);
        return value;
    }

    bool read_raw(uintptr_t address, void* buffer, size_t size) const
    {
        if (!address || !ok() || !buffer || !size)
            return false;
        SIZE_T read = 0;
        return ReadProcessMemory(handle_, reinterpret_cast<LPCVOID>(address), buffer, size, &read) && read == size;
    }

private:
    HANDLE handle_ = nullptr;
    DWORD pid_ = 0;
};
