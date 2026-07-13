#pragma once

#include <atomic>
#include <string>

// ─────────────────────────────────────────────────────────────────────────────
// IDGenerator — generates unique, chronologically-sortable IDs.
//
// Formats:
//   User        USR-YYYYMMDD-XXXX
//   Account     ACC-YYYYMMDD-XXXX
//   Transaction TXN-YYYYMMDDHHMMSS-XXXX
//
// The per-type counter is persisted in data/counters.json so IDs do not
// repeat after a restart.
// ─────────────────────────────────────────────────────────────────────────────

namespace novabanc {

class IDGenerator {
public:
    IDGenerator() = delete;

    /// Must be called once at startup; reads counters from disk.
    static void initialize(const std::string& countersFilePath);

    static std::string generateUserID();
    static std::string generateAccountID();
    static std::string generateTransactionID();

private:
    static std::atomic<int> s_userCounter;
    static std::atomic<int> s_accountCounter;
    static std::atomic<int> s_txnCounter;
    static std::string      s_countersPath;

    static void saveCounters();
    static std::string buildID(const std::string& prefix,
                               const std::string& dateStr,
                               int counter);
};

} // namespace novabanc
