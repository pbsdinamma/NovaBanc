#include "IDGenerator.h"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "Filesystem.h"
#include "Logger.h"
#include "Utils.h"
#include <nlohmann/json.hpp>

namespace novabanc {

// ─── Static member definitions ────────────────────────────────────────────────

std::atomic<int> IDGenerator::s_userCounter{1};
std::atomic<int> IDGenerator::s_accountCounter{1};
std::atomic<int> IDGenerator::s_txnCounter{1};
std::string      IDGenerator::s_countersPath;

// ─── Helpers ──────────────────────────────────────────────────────────────────

namespace {

// Returns "YYYYMMDD" from the current local time.
std::string dateStamp() {
    auto now  = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_val{};
#ifdef _WIN32
    tm_val = *std::localtime(&t);
#else
    localtime_r(&t, &tm_val);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_val, "%Y%m%d");
    return oss.str();
}

// Returns "YYYYMMDDHHMMSS" from the current local time.
std::string dateTimeStamp() {
    auto now  = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_val{};
#ifdef _WIN32
    tm_val = *std::localtime(&t);
#else
    localtime_r(&t, &tm_val);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_val, "%Y%m%d%H%M%S");
    return oss.str();
}

} // anonymous namespace

// ─── Public API ───────────────────────────────────────────────────────────────

void IDGenerator::initialize(const std::string& countersFilePath) {
    s_countersPath = countersFilePath;

    if (!fs::exists(countersFilePath)) {
        // First run — counters stay at 1.
        return;
    }

    try {
        std::ifstream file(countersFilePath);
        nlohmann::json j;
        file >> j;
        s_userCounter.store(j.value("user_counter", 1));
        s_accountCounter.store(j.value("account_counter", 1));
        s_txnCounter.store(j.value("txn_counter", 1));
    } catch (...) {
        // Corrupted counter file — start from 1 (safe default).
    }
}

std::string IDGenerator::buildID(const std::string& prefix,
                                 const std::string& dateStr,
                                 int counter) {
    std::ostringstream oss;
    oss << prefix << "-" << dateStr << "-"
        << std::setfill('0') << std::setw(4) << counter;
    return oss.str();
}

std::string IDGenerator::generateUserID() {
    int counter = s_userCounter.fetch_add(1);
    saveCounters();
    return buildID("USR", dateStamp(), counter);
}

std::string IDGenerator::generateAccountID() {
    int counter = s_accountCounter.fetch_add(1);
    saveCounters();
    return buildID("ACC", dateStamp(), counter);
}

std::string IDGenerator::generateTransactionID() {
    int counter = s_txnCounter.fetch_add(1);
    saveCounters();
    return buildID("TXN", dateTimeStamp(), counter);
}

void IDGenerator::saveCounters() {
    if (s_countersPath.empty()) return;

    try {
        fs::create_directories(
            fs::path(s_countersPath).parent_path());

        nlohmann::json j;
        j["user_counter"]    = s_userCounter.load();
        j["account_counter"] = s_accountCounter.load();
        j["txn_counter"]     = s_txnCounter.load();

        std::ofstream file(s_countersPath);
        file << j.dump(4);
    } catch (...) {
        // Counter file write failure is non-fatal for a single session.
    }
}

} // namespace novabanc
