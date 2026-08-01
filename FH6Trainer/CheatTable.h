#pragma once
#include "CheatEngine.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace FH6 {

enum class CheatKind {
    TogglePatch,   // patch bytes (e.g. freeze AI jump)
    ValueSet,      // write a fixed value to a resolved address
    PointerSet     // follow pointer chain then write value
};

struct CheatEntry {
    std::string        id;
    std::string        label;
    CheatKind          kind;
    CheatSignature     signature;
    int64_t            setValue;        // value to write for ValueSet / PointerSet
    bool               active = false;
    uintptr_t          resolvedAddress = 0;
    std::vector<uint8_t> savedBytes;    // original bytes for TogglePatch restore
};

class CheatTable {
public:
    void LoadDefaults();

    const std::vector<CheatEntry>& Entries() const { return m_entries; }
    std::vector<CheatEntry>& MutableEntries() { return m_entries; }

    CheatEntry* Find(const std::string& id);
    void Add(CheatEntry e) { m_entries.push_back(std::move(e)); }

    // Hot-reload: replace signatures without losing active state where possible.
    void ReloadSignatures(const std::vector<CheatEntry>& updated);

private:
    std::vector<CheatEntry> m_entries;

    static AobPattern Pat(const std::vector<uint8_t>& bytes,
                          const std::vector<bool>& mask) {
        return { bytes, mask };
    }
};

}  // namespace FH6
