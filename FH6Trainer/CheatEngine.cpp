#include "CheatEngine.h"
#include <psapi.h>
#include <algorithm>

#pragma comment(lib, "psapi.lib")

namespace FH6 {

CheatEngine::CheatEngine(DWORD pid) : m_pid(pid) {}

CheatEngine::~CheatEngine() {
    if (m_process) {
        CloseHandle(m_process);
        m_process = nullptr;
    }
}

bool CheatEngine::Attach() {
    m_process = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE |
                            PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION,
                            FALSE, m_pid);
    if (!m_process) return false;
    CacheRegions();
    return true;
}

void CheatEngine::CacheRegions() {
    m_regions.clear();
    MEMORY_BASIC_INFORMATION mbi = {};
    uintptr_t addr = 0;
    while (VirtualQueryEx(m_process, (LPCVOID)addr, &mbi, sizeof(mbi))) {
        if (mbi.State == MEM_COMMIT &&
            (mbi.Protect & (PAGE_READWRITE | PAGE_EXECUTE_READWRITE |
                            PAGE_EXECUTE_READ | PAGE_READONLY))) {
            m_regions.push_back({ (uintptr_t)mbi.BaseAddress, mbi.RegionSize });
        }
        addr = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
        if (addr == 0) break;  // wraparound
    }
}

uintptr_t CheatEngine::GetModuleBase(const std::string& moduleName) {
    HMODULE modules[1024];
    DWORD needed = 0;
    if (!EnumProcessModules(m_process, modules, sizeof(modules), &needed)) return 0;
    int count = needed / sizeof(HMODULE);
    for (int i = 0; i < count; ++i) {
        char name[MAX_PATH] = {};
        if (GetModuleBaseNameA(m_process, modules[i], name, sizeof(name))) {
            std::string n(name);
            std::transform(n.begin(), n.end(), n.begin(), ::tolower);
            std::string target(moduleName);
            std::transform(target.begin(), target.end(), target.begin(), ::tolower);
            if (n == target) return (uintptr_t)modules[i];
        }
    }
    return 0;
}

bool CheatEngine::Read(uintptr_t address, void* buffer, size_t size) {
    SIZE_T read = 0;
    return ReadProcessMemory(m_process, (LPCVOID)address, buffer, size, &read) &&
           read == size;
}

bool CheatEngine::Write(uintptr_t address, const void* buffer, size_t size) {
    SIZE_T written = 0;
    return WriteProcessMemory(m_process, (LPVOID)address, buffer, size, &written) &&
           written == size;
}

uintptr_t CheatEngine::AobScan(const AobPattern& pat, uintptr_t start, uintptr_t end) {
    if (pat.bytes.size() != pat.mask.size() || pat.bytes.empty()) return 0;
    size_t plen = pat.bytes.size();

    std::vector<uint8_t> chunk(0x10000);
    for (uintptr_t addr = start; addr + plen < end; addr += chunk.size() - plen) {
        SIZE_T read = 0;
        size_t toRead = std::min(chunk.size(), (size_t)(end - addr));
        if (!ReadProcessMemory(m_process, (LPCVOID)addr, chunk.data(),
                               toRead, &read) || read == 0) {
            continue;
        }
        for (size_t i = 0; i + plen <= read; ++i) {
            bool match = true;
            for (size_t j = 0; j < plen; ++j) {
                if (pat.mask[j] && chunk[i + j] != pat.bytes[j]) {
                    match = false;
                    break;
                }
            }
            if (match) return addr + i;
        }
    }
    return 0;
}

uintptr_t CheatEngine::Resolve(const CheatSignature& sig) {
    uintptr_t moduleBase = GetModuleBase(sig.moduleName);
    if (!moduleBase) return 0;

    switch (sig.type) {
        case ScanType::StaticOffset:
            return moduleBase + sig.staticOffset;

        case ScanType::AOB: {
            for (const auto& r : m_regions) {
                uintptr_t found = AobScan(sig.aob, r.base, r.base + r.size);
                if (found) return found;
            }
            return 0;
        }

        case ScanType::PointerChain: {
            uintptr_t root = 0;
            for (const auto& r : m_regions) {
                root = AobScan(sig.chain.root, r.base, r.base + r.size);
                if (root) break;
            }
            if (!root) return 0;

            // The root AOB hit is followed by a mov rax/eax, [xxx] or lea whose
            // operand points to the static player struct. Walk the hop chain.
            uintptr_t ptr = root + sig.chain.offsets[0];
            uintptr_t value = 0;
            for (size_t i = 1; i < sig.chain.offsets.size(); ++i) {
                if (!Read(ptr, &value, sizeof(value))) return 0;
                ptr = value + sig.chain.offsets[i];
            }
            return ptr;
        }
    }
    return 0;
}

bool CheatEngine::PatchToggle(uintptr_t address, const std::vector<uint8_t>& patch,
                              std::vector<uint8_t>& originalOut) {
    originalOut.resize(patch.size());
    if (!Read(address, originalOut.data(), patch.size())) return false;
    DWORD oldProtect = 0;
    if (!VirtualProtectEx(m_process, (LPVOID)address, patch.size(),
                          PAGE_EXECUTE_READWRITE, &oldProtect)) return false;
    bool ok = Write(address, patch.data(), patch.size());
    DWORD tmp = 0;
    VirtualProtectEx(m_process, (LPVOID)address, patch.size(), oldProtect, &tmp);
    return ok;
}

bool CheatEngine::RestoreToggle(uintptr_t address, const std::vector<uint8_t>& original) {
    DWORD oldProtect = 0;
    if (!VirtualProtectEx(m_process, (LPVOID)address, original.size(),
                          PAGE_EXECUTE_READWRITE, &oldProtect)) return false;
    bool ok = Write(address, original.data(), original.size());
    DWORD tmp = 0;
    VirtualProtectEx(m_process, (LPVOID)address, original.size(), oldProtect, &tmp);
    return ok;
}

}  // namespace FH6
