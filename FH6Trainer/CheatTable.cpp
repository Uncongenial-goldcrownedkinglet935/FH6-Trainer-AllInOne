#include "CheatTable.h"

namespace FH6 {

void CheatTable::LoadDefaults() {
    m_entries.clear();

    // ---- Infinite credits (PointerChain) ----
    // Root AOB locates the player wallet struct; offsets walk to the credits field.
    CheatEntry credits{};
    credits.id = "infinite_credits";
    credits.label = "Infinite Credits";
    credits.kind = CheatKind::PointerSet;
    credits.signature.type = ScanType::PointerChain;
    credits.signature.moduleName = "forza_core.dll";
    credits.signature.chain.root = Pat(
        { 0x48, 0x8B, 0x05, 0x00, 0x00, 0x00, 0x00, 0x48, 0x85, 0xC0, 0x74, 0x0A },
        { 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1 });
    credits.signature.chain.offsets = { 0x03, 0x150, 0x28, 0x10 };
    credits.setValue = 999999999;
    m_entries.push_back(std::move(credits));

    // ---- Unlimited XP (PointerChain) ----
    CheatEntry xp{};
    xp.id = "unlimited_xp";
    xp.label = "Unlimited XP";
    xp.kind = CheatKind::PointerSet;
    xp.signature.type = ScanType::PointerChain;
    xp.signature.moduleName = "forza_core.dll";
    xp.signature.chain.root = Pat(
        { 0xF3, 0x0F, 0x11, 0x86, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x86 },
        { 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1 });
    xp.signature.chain.offsets = { 0x07, 0x210, 0x40 };
    xp.setValue = 9999999;
    m_entries.push_back(std::move(xp));

    // ---- Freeze AI (TogglePatch) ----
    // NOP the AI update branch to hold rivals and traffic in place.
    CheatEntry freezeAi{};
    freezeAi.id = "freeze_ai";
    freezeAi.label = "Freeze AI";
    freezeAi.kind = CheatKind::TogglePatch;
    freezeAi.signature.type = ScanType::AOB;
    freezeAi.signature.moduleName = "forza_ai.dll";
    freezeAi.signature.aob = Pat(
        { 0x40, 0x57, 0x48, 0x83, 0xEC, 0x30, 0x48, 0x8B, 0xF9, 0xE8 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 });
    freezeAi.signature.patchSize = 5;
    freezeAi.signature.patchedBytes = { 0x90, 0x90, 0x90, 0x90, 0x90 };  // 5x NOP
    m_entries.push_back(std::move(freezeAi));

    // ---- Speed hack (TogglePatch on speed clamp) ----
    CheatEntry speed{};
    speed.id = "speed_hack";
    speed.label = "Speed Hack";
    speed.kind = CheatKind::TogglePatch;
    speed.signature.type = ScanType::AOB;
    speed.signature.moduleName = "forza_core.dll";
    speed.signature.aob = Pat(
        { 0xF3, 0x0F, 0x5E, 0x86, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x2E, 0xC8 },
        { 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1 });
    speed.signature.patchSize = 3;
    speed.signature.patchedBytes = { 0x90, 0x90, 0x90 };
    m_entries.push_back(std::move(speed));

    // ---- Grip hack (ValueSet on tire friction scalar) ----
    CheatEntry grip{};
    grip.id = "grip_hack";
    grip.label = "Grip Hack";
    grip.kind = CheatKind::PointerSet;
    grip.signature.type = ScanType::PointerChain;
    grip.signature.moduleName = "forza_core.dll";
    grip.signature.chain.root = Pat(
        { 0x48, 0x8D, 0x8F, 0x00, 0x00, 0x00, 0x00, 0xF3, 0x0F, 0x10 },
        { 1, 1, 1, 0, 0, 0, 0, 1, 1, 1 });
    grip.signature.chain.offsets = { 0x03, 0x3C0, 0x18, 0x04 };
    // IEEE-754 float 2.0 as int64 bits: 0x40000000
    grip.setValue = 0x40000000;
    m_entries.push_back(std::move(grip));
}

CheatEntry* CheatTable::Find(const std::string& id) {
    for (auto& e : m_entries) {
        if (e.id == id) return &e;
    }
    return nullptr;
}

void CheatTable::ReloadSignatures(const std::vector<CheatEntry>& updated) {
    // Preserve active flags where the id still exists in the new table.
    std::unordered_map<std::string, bool> wasActive;
    for (const auto& e : m_entries) {
        wasActive[e.id] = e.active;
    }
    m_entries = updated;
    for (auto& e : m_entries) {
        auto it = wasActive.find(e.id);
        if (it != wasActive.end()) e.active = it->second;
    }
}

}  // namespace FH6
