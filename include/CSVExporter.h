#pragma once

#include <string>
#include <vector>

// Forward declarations.
namespace novabanc {
    class User;
    class Account;
    class Transaction;
}

// ─────────────────────────────────────────────────────────────────────────────
// CSVExporter — static utilities for generating CSV reports.
//
// No state. Pass data in, get a file out. The `reports/` directory is created
// automatically if it doesn't exist.
// ─────────────────────────────────────────────────────────────────────────────

namespace novabanc {

class CSVExporter {
public:
    CSVExporter() = delete;

    /// Export all users. Returns false on file I/O error.
    static bool exportUsers(const std::vector<User*>& users,
                            const std::string& path);

    /// Export a flat list of transactions.
    static bool exportTransactions(const std::vector<Transaction>& txns,
                                   const std::string& path);

    /// Export a one-row-per-account summary.
    static bool exportAccountSummary(const std::vector<Account*>& accounts,
                                     const std::string& path);

    /// Export all transactions for a single account (account statement).
    static bool exportUserStatement(const Account& account,
                                    const std::string& path);

private:
    static void ensureParentDir(const std::string& path);
    static std::string csvField(const std::string& s);
};

} // namespace novabanc
