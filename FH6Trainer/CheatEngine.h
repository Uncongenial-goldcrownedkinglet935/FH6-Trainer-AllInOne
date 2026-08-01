#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <Windows.h>

namespace FH6 {

enum class ScanType {
    AOB,            // array-of-bytes pattern scan
    PointerChain,   // multi-level pointer resolution
    StaticOffset    // module base + offset
};

struct AobPattern {
    std::vector<uint8_t> bytes;
    std::vector<bool>    mask;   // true = must match, false = wildcard
};

struct PointerHop {
    AobPattern        root;      // starting AOB to find the base
    std::vector<int>  offsets;   // multi-level pointer offsets
};

struct CheatSignature {
    std::string  name;
    ScanType     type;
    AobPattern   aob;            // used when type == AOB
    PointerHop   chain;          // used when type == PointerChain
    uintptr_t    staticOffset;   // used when type == StaticOffset (relative to module)
    std::string  moduleName;     // target module (e.g. "forza_core.dll")
    int          patchSize;      // bytes to patch for toggle cheats
    std::vector<uint8_t> originalBytes;
    std::vector<uint8_t> patchedBytes;
};

class CheatEngine {
public:
    explicit CheatEngine(DWORD pid);
    ~CheatEngine();

    bool Attach();
    bool IsAttached() const { return m_process != nullptr; }

    uintptr_t GetModuleBase(const std::string& moduleName);

    // Resolve a cheat signature to a concrete address in the target process.
    uintptr_t Resolve(const CheatSignature& sig);

    // Read/write arbitrary memory at a resolved address.
    bool Read(uintptr_t address, void* buffer, size_t size);
    bool Write(uintptr_t address, const void* buffer, size_t size);

    // AOB scan within a memory region (returns 0 if not found).
    uintptr_t AobScan(const AobPattern& pat, uintptr_t start, uintptr_t end);

    // Patch toggle bytes and store the original for restore.
    bool PatchToggle(uintptr_t address, const std::vector<uint8_t>& patch,
                     std::vector<uint8_t>& originalOut);
    bool RestoreToggle(uintptr_t address, const std::vector<uint8_t>& original);

private:
    struct Region { uintptr_t base; size_t size; };

    DWORD               m_pid = 0;
    HANDLE              m_process = nullptr;
    std::vector<Region> m_regions;

    void CacheRegions();
};

}  // namespace FH6
