#include "CSVExporter.h"

#include <fstream>
#include <iomanip>
#include <sstream>

#include "Account.h"
#include "Filesystem.h"
#include "Transaction.h"
#include "User.h"

namespace novabanc {

// ── Private helpers ───────────────────────────────────────────────────────────

void CSVExporter::ensureParentDir(const std::string& path) {
    auto parent = fs::path(path).parent_path();
    if (!parent.empty()) {
        fs::create_directories(parent);
    }
}

std::string CSVExporter::csvField(const std::string& s) {
    // Wrap in double quotes if the field contains a comma or a double-quote.
    if (s.find(',') != std::string::npos || s.find('"') != std::string::npos) {
        std::string escaped;
        for (char c : s) {
            if (c == '"') escaped += '"'; // escape embedded quotes
            escaped += c;
        }
        return "\"" + escaped + "\"";
    }
    return s;
}

// ── Export methods ────────────────────────────────────────────────────────────

bool CSVExporter::exportUsers(const std::vector<User*>& users,
                              const std::string& path) {
    ensureParentDir(path);
    std::ofstream file(path);
    if (!file.is_open()) return false;

    file << User::csvHeader() << "\n";
    for (const User* user : users) {
        if (user) file << user->toCSVRow() << "\n";
    }
    return true;
}

bool CSVExporter::exportTransactions(const std::vector<Transaction>& txns,
                                     const std::string& path) {
    ensureParentDir(path);
    std::ofstream file(path);
    if (!file.is_open()) return false;

    file << Transaction::csvHeader() << "\n";
    for (const auto& txn : txns) {
        file << txn.toCSVRow() << "\n";
    }
    return true;
}

bool CSVExporter::exportAccountSummary(const std::vector<Account*>& accounts,
                                       const std::string& path) {
    ensureParentDir(path);
    std::ofstream file(path);
    if (!file.is_open()) return false;

    file << "account_id,user_id,type,balance,status,opened_date,transaction_count\n";
    for (const Account* acc : accounts) {
        if (!acc) continue;
        std::ostringstream row;
        row << std::fixed << std::setprecision(2);
        row << csvField(acc->getAccountID())  << ","
            << csvField(acc->getUserID())     << ","
            << acc->getAccountType()          << ","
            << acc->getBalance()              << ","
            << acc->getStatusString()         << ","
            << csvField(acc->getOpenedDate()) << ","
            << acc->getTransactions().size();
        file << row.str() << "\n";
    }
    return true;
}

bool CSVExporter::exportUserStatement(const Account& account,
                                      const std::string& path) {
    ensureParentDir(path);
    std::ofstream file(path);
    if (!file.is_open()) return false;

    // Statement header block.
    file << "Account Statement\n";
    file << "Account ID: " << account.getAccountID() << "\n";
    file << "Account Type: " << account.getAccountType() << "\n";
    file << "Current Balance: " << std::fixed << std::setprecision(2)
         << account.getBalance() << "\n";
    file << "\n";
    file << Transaction::csvHeader() << "\n";

    for (const auto& txn : account.getTransactions()) {
        file << txn.toCSVRow() << "\n";
    }
    return true;
}

} // namespace novabanc
