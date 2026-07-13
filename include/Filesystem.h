#pragma once

#include <string>
#include <cstdlib>
#include <fstream>
#include <system_error>
#include <cstdio>

namespace fs {

struct path {
    std::string p;
    path(const std::string& p_) : p(p_) {}
    path(const char* p_) : p(p_) {}
    
    std::string parent_path() const {
        size_t pos = p.find_last_of("/\\");
        if (pos == std::string::npos) return "";
        return p.substr(0, pos);
    }
    
    bool empty() const { return p.empty(); }
    std::string string() const { return p; }
};

inline bool exists(const std::string& path) {
    std::ifstream f(path.c_str());
    return f.good();
}

inline void create_directories(const std::string& p) {
    if (p.empty()) return;
    std::string cmd = "mkdir \"" + p + "\" 2>nul";
    std::system(cmd.c_str());
}

inline void create_directories(const path& p) {
    create_directories(p.string());
}

inline void remove(const std::string& p) {
    std::remove(p.c_str());
}

inline void remove_all(const std::string& p, std::error_code&) {
    std::string cmd = "rmdir /S /Q \"" + p + "\" 2>nul";
    std::system(cmd.c_str());
}

inline void remove_all(const std::string& p) {
    std::error_code ec;
    remove_all(p, ec);
}

} // namespace fs
