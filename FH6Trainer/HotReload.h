#pragma once
#include <string>
#include <vector>

namespace FH6 {

class HotReload {
public:
    explicit HotReload(const std::string& configPath);

    // Check the config file mtime; reload if changed. Returns true if reloaded.
    bool CheckAndReload(class CheatTable& table);

    // Force a reload now.
    bool Reload(class CheatTable& table);

private:
    std::string m_path;
    int64_t     m_lastMtime = 0;

    int64_t FileMtime();
    bool    ParseConfig(const std::string& content, class CheatTable& table);
};

}  // namespace FH6
