#include "sdk/memory.hpp"

#include <TlHelp32.h>

bool Memory::attach(const wchar_t* process_name)
{
    close();

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
        return false;

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);
    DWORD found = 0;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, process_name) == 0) {
                found = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);

    if (!found)
        return false;

    handle_ = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, found);
    if (!handle_)
        return false;

    pid_ = found;
    return true;
}

void Memory::close()
{
    if (handle_ && handle_ != INVALID_HANDLE_VALUE)
        CloseHandle(handle_);
    handle_ = nullptr;
    pid_ = 0;
}

uintptr_t Memory::module_base(const wchar_t* module_name) const
{
    if (!ok())
        return 0;

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid_);
    if (snap == INVALID_HANDLE_VALUE)
        return 0;

    MODULEENTRY32W me{};
    me.dwSize = sizeof(me);
    uintptr_t base = 0;
    if (Module32FirstW(snap, &me)) {
        do {
            if (_wcsicmp(me.szModule, module_name) == 0) {
                base = reinterpret_cast<uintptr_t>(me.modBaseAddr);
                break;
            }
        } while (Module32NextW(snap, &me));
    }
    CloseHandle(snap);
    return base;
}
