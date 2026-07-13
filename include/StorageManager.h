#pragma once

#include <map>
#include <memory>
#include <string>

// Forward declarations to avoid circular includes.
namespace novabanc {
    class User;
    class Account;
    class Bank;
}

// ─────────────────────────────────────────────────────────────────────────────
// StorageManager — reads and writes all JSON persistence files.
//
// Only Bank calls StorageManager directly. This separation means that if the
// storage format ever needs to change (e.g., switch to SQLite), only this
// class needs updating.
// ─────────────────────────────────────────────────────────────────────────────

namespace novabanc {

class StorageManager {
public:
    explicit StorageManager(const std::string& dataDirectory);

    // ── Save methods ─────────────────────────────────────────────────────────
    [[nodiscard]] bool saveUsers(const std::map<std::string, User>& users);
    [[nodiscard]] bool saveAccounts(const std::map<std::string, std::shared_ptr<Account>>& accounts);
    [[nodiscard]] bool saveTransactions(const std::map<std::string, std::shared_ptr<Account>>& accounts);

    // ── Load methods ─────────────────────────────────────────────────────────
    [[nodiscard]] bool loadUsers(std::map<std::string, User>& users,
                                 std::map<std::string, std::string>& usernameIndex);
    [[nodiscard]] bool loadAccounts(std::map<std::string, std::shared_ptr<Account>>& accounts);
    [[nodiscard]] bool loadTransactions(std::map<std::string, std::shared_ptr<Account>>& accounts);

    // ── Convenience ──────────────────────────────────────────────────────────
    [[nodiscard]] bool saveAll(const Bank& bank);
    [[nodiscard]] bool loadAll(Bank& bank);

    const std::string& getDataDirectory() const { return m_dataDir; }

private:
    std::string m_dataDir;

    std::string usersFilePath()        const;
    std::string accountsFilePath()     const;
    std::string transactionsFilePath() const;

    void ensureDataDirectory() const;
};

} // namespace novabanc
