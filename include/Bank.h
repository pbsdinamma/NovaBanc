#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "User.h"
#include "Account.h"
#include "Logger.h"
#include "StorageManager.h"

// ─────────────────────────────────────────────────────────────────────────────
// Bank — the central repository and orchestrator.
//
// Every banking operation flows through Bank. It owns all Users and Accounts,
// delegates persistence to StorageManager, and logs every significant event.
// The UI layer never touches User or Account objects directly.
// ─────────────────────────────────────────────────────────────────────────────

namespace novabanc {

struct BankStats {
    int    totalUsers;
    int    lockedUsers;
    int    totalAccounts;
    int    activeAccounts;
    int    frozenAccounts;
    int    savingsAccounts;
    int    currentAccounts;
    double totalBalance;
    int    totalTransactions;
};

class Bank {
public:
    Bank(const std::string& bankName,
         StorageManager&    storage,
         Logger&            logger);

    // ── Data access (used by StorageManager) ─────────────────────────────────
    const std::map<std::string, User>&                    getUsers()    const { return m_users; }
    const std::map<std::string, std::shared_ptr<Account>>& getAccounts() const { return m_accounts; }

    // ── Persistence ───────────────────────────────────────────────────────────
    [[nodiscard]] bool saveAll();
    [[nodiscard]] bool loadAll();

    // ── User management ───────────────────────────────────────────────────────
    User& createUser(const std::string& username,
                     const std::string& password,
                     const std::string& firstName,
                     const std::string& lastName,
                     const std::string& email,
                     const std::string& phone,
                     int                age,
                     const std::string& address);

    bool deleteUser(const std::string& userID);

    User*              findUserByID(const std::string& userID);
    User*              findUserByUsername(const std::string& username);
    std::vector<User*> searchUsers(const std::string& query);
    std::vector<User*> getAllUsers();

    bool updateUserEmail(const std::string& userID, const std::string& email);
    bool updateUserPhone(const std::string& userID, const std::string& phone);

    // ── Account management ────────────────────────────────────────────────────
    std::shared_ptr<Account> createSavingsAccount(const std::string& userID,
                                                   double initialDeposit);
    std::shared_ptr<Account> createCurrentAccount(const std::string& userID,
                                                   double initialDeposit);
    bool closeAccount(const std::string& accountID);
    bool freezeAccount(const std::string& accountID);
    bool unfreezeAccount(const std::string& accountID);

    Account*              findAccountByID(const std::string& accountID);
    std::vector<Account*> getAccountsByUserID(const std::string& userID);
    std::vector<Account*> getAllAccounts();

    // ── Financial operations ──────────────────────────────────────────────────
    [[nodiscard]] bool deposit(const std::string& accountID,
                               double amount,
                               const std::string& description = "");

    [[nodiscard]] bool withdraw(const std::string& accountID,
                                double amount,
                                const std::string& description = "");

    /// Atomically transfers `amount` from `fromID` to `toID`.
    /// Reverses the debit if the credit fails.
    [[nodiscard]] bool transfer(const std::string& fromID,
                                const std::string& toID,
                                double amount);

    std::vector<Transaction> getTransactionHistory(const std::string& accountID) const;

    // ── Admin operations ──────────────────────────────────────────────────────
    void applyMonthlyInterestAll();

    BankStats getStats() const;

    const std::string& getBankName() const { return m_bankName; }

    // StorageManager needs direct map access for load operations.
    friend class StorageManager;

private:
    std::string                                    m_bankName;
    std::map<std::string, User>                    m_users;
    std::map<std::string, std::shared_ptr<Account>>m_accounts;
    std::map<std::string, std::string>             m_usernameIndex; // username → userID
    StorageManager&                                m_storage;
    Logger&                                        m_logger;
};

} // namespace novabanc
