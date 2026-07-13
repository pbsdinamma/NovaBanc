#pragma once

#include <string>
#include <vector>
#include "Transaction.h"
#include <nlohmann/json.hpp>

// ─────────────────────────────────────────────────────────────────────────────
// Account — abstract base class for all account types.
//
// Common state (balance, transactions, status, daily limit logic) lives here.
// Account-type-specific behavior — minimum balance, interest rate, withdrawal
// rules — is delegated to SavingsAccount and CurrentAccount via pure virtuals.
//
// Design intent: Account is open for extension (new subclasses) but closed for
// modification. Adding a FixedDeposit type means writing a new subclass, not
// changing Account.
// ─────────────────────────────────────────────────────────────────────────────

namespace novabanc {

enum class AccountStatus {
    ACTIVE,
    FROZEN,
    CLOSED
};

class Account {
public:
    virtual ~Account() = default;

    // ── Pure virtual interface ────────────────────────────────────────────────
    virtual double      getMinimumBalance() const = 0;
    virtual double      getInterestRate()   const = 0;
    virtual std::string getAccountType()    const = 0;

    /// Returns true iff this account may withdraw `amount` right now.
    virtual bool canWithdraw(double amount) const = 0;

    virtual nlohmann::json toJson() const = 0;

    // ── Concrete operations ───────────────────────────────────────────────────

    /// Validates amount > 0, updates balance, records a DEPOSIT transaction.
    /// Returns true on success; throws InvalidInputException on bad amount,
    /// AccountFrozenException if frozen.
    [[nodiscard]] virtual bool deposit(double amount,
                                       const std::string& description = "");

    /// Validates via canWithdraw(), updates balance, records a WITHDRAWAL.
    /// Throws InsufficientFundsException, AccountFrozenException, or
    /// InvalidInputException as appropriate.
    [[nodiscard]] virtual bool withdraw(double amount,
                                        const std::string& description = "");

    void freeze();
    void unfreeze();

    /// Closes the account. Returns false if balance != 0 or already closed.
    bool close();

    // ── Getters ───────────────────────────────────────────────────────────────
    const std::string& getAccountID()  const { return m_accountID; }
    const std::string& getUserID()     const { return m_userID; }
    double             getBalance()    const { return m_balance; }
    AccountStatus      getStatus()     const { return m_status; }
    const std::string& getOpenedDate() const { return m_openedDate; }
    double             getDailyWithdrawnToday() const { return m_dailyWithdrawnToday; }

    bool isActive() const { return m_status == AccountStatus::ACTIVE; }
    bool isFrozen() const { return m_status == AccountStatus::FROZEN; }
    bool isClosed() const { return m_status == AccountStatus::CLOSED; }

    std::string getStatusString() const;

    const std::vector<Transaction>& getTransactions() const { return m_transactions; }

    /// Used by StorageManager when deserializing persisted transactions.
    void loadTransaction(const Transaction& txn) { m_transactions.push_back(txn); }

    /// Default daily withdrawal limit shared by all account types (subclasses
    /// may shadow this with a higher value).
    static constexpr double DAILY_LIMIT = 10000.0;

protected:
    /// Protected constructor — only subclasses instantiate Account.
    Account(const std::string& accountID,
            const std::string& userID,
            double             balance,
            AccountStatus      status,
            const std::string& openedDate,
            double             dailyWithdrawnToday,
            const std::string& lastWithdrawDate);

    std::string              m_accountID;
    std::string              m_userID;
    double                   m_balance;
    AccountStatus            m_status;
    std::string              m_openedDate;
    std::vector<Transaction> m_transactions;
    double                   m_dailyWithdrawnToday;
    std::string              m_lastWithdrawDate;

    /// Subclasses populate additional fields into this JSON object.
    virtual nlohmann::json typeSpecificJson() const = 0;

    /// Resets daily limit counter if the date has changed since last withdraw.
    void refreshDailyLimit();
};

} // namespace novabanc
