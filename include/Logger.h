#pragma once

#include <fstream>
#include <string>

// ─────────────────────────────────────────────────────────────────────────────
// Logger — single-instance file logger, passed by reference.
//
// Not a singleton. The Logger is constructed in main() and injected wherever
// it is needed. Log format:
//   [2025-06-01 14:32:01] [INFO] message text
// ─────────────────────────────────────────────────────────────────────────────

namespace novabanc {

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

class Logger {
public:
    /// Opens `filePath` for append. Creates parent directories if needed.
    explicit Logger(const std::string& filePath,
                    LogLevel minLevel = LogLevel::INFO);

    ~Logger();

    // Non-copyable (file handle ownership).
    Logger(const Logger&)            = delete;
    Logger& operator=(const Logger&) = delete;

    void log(LogLevel level, const std::string& message);

    void debug(const std::string& msg);
    void info(const std::string& msg);
    void warn(const std::string& msg);
    void error(const std::string& msg);

private:
    std::ofstream m_file;
    LogLevel      m_minLevel;

    static std::string levelToString(LogLevel level);
    static std::string currentTimestamp();
};

} // namespace novabanc
