#include "Logger.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "Filesystem.h"

namespace novabanc {

Logger::Logger(const std::string& filePath, LogLevel minLevel)
    : m_minLevel(minLevel) {
    fs::create_directories(fs::path(filePath).parent_path());
    m_file.open(filePath, std::ios::app);
    if (!m_file.is_open()) {
        // Fall back to stderr — logging failure should never crash the app.
        std::cerr << "[Logger] Could not open log file: " << filePath << "\n";
    }
}

Logger::~Logger() {
    if (m_file.is_open()) m_file.close();
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR:   return "ERROR";
    }
    return "UNKNOWN";
}

std::string Logger::currentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_val{};
#ifdef _WIN32
    tm_val = *std::localtime(&t);
#else
    localtime_r(&t, &tm_val);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_val, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

void Logger::log(LogLevel level, const std::string& message) {
    if (static_cast<int>(level) < static_cast<int>(m_minLevel)) return;

    std::string line = "[" + currentTimestamp() + "] [" +
                       levelToString(level) + "] " + message;

    if (m_file.is_open()) {
        m_file << line << "\n";
        m_file.flush();
    }
}

void Logger::debug(const std::string& msg)   { log(LogLevel::DEBUG,   msg); }
void Logger::info(const std::string& msg)    { log(LogLevel::INFO,    msg); }
void Logger::warn(const std::string& msg)    { log(LogLevel::WARNING, msg); }
void Logger::error(const std::string& msg)   { log(LogLevel::ERROR,   msg); }

} // namespace novabanc
