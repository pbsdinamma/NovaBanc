#include "CurrentAccount.h"
#include "Exceptions.h"
#include "IDGenerator.h"
#include "Utils.h"

namespace novabanc {

// ── Constructor ───────────────────────────────────────────────────────────────

CurrentAccount::CurrentAccount(const std::string& accountID,
                               const std::string& userID,
                               double             balance,
                               AccountStatus      status,
                               const std::string& openedDate,
                               double             dailyWithdrawnToday,
                               const std::string& lastWithdrawDate)
    : Account(accountID, userID, balance, status,
              openedDate, dailyWithdrawnToday, lastWithdrawDate) {}

// ── canWithdraw ───────────────────────────────────────────────────────────────

bool CurrentAccount::canWithdraw(double amount) const {
    if (!isActive()) return false;
    // Allow overdraft up to OVERDRAFT_LIMIT below zero.
    if (m_balance - amount < -OVERDRAFT_LIMIT) return false;
    if (m_dailyWithdrawnToday + amount > DAILY_LIMIT) return false;
    return true;
}

// ── Override withdraw to use CurrentAccount's DAILY_LIMIT ─────────────────────

bool CurrentAccount::withdraw(double amount, const std::string& description) {
    // Refresh daily counters with today's date.
    refreshDailyLimit();

    if (amount <= 0.0) {
        throw InvalidInputException("Withdrawal amount must be a positive number.");
    }
    if (isFrozen()) {
        throw AccountFrozenException("Account " + m_accountID + " is frozen.");
    }
    if (isClosed()) {
        throw InvalidInputException("Cannot withdraw from a closed account.");
    }
    if (m_balance - amount < -OVERDRAFT_LIMIT) {
        throw InsufficientFundsException(
            "Withdrawal would exceed overdraft limit of Rs.5,000.00.");
    }
    if (m_dailyWithdrawnToday + amount > DAILY_LIMIT) {
        throw InsufficientFundsException(
            "Daily withdrawal limit of Rs.25,000.00 would be exceeded.");
    }

    m_balance             -= amount;
    m_dailyWithdrawnToday += amount;

    std::string ts   = utils::getCurrentTimestamp();
    std::string id   = IDGenerator::generateTransactionID();
    std::string desc = description.empty() ? "Withdrawal" : description;

    m_transactions.emplace_back(id, m_accountID,
                                TransactionType::WITHDRAWAL,
                                amount, m_balance, ts, desc);
    return true;
}

// ── JSON serialization ────────────────────────────────────────────────────────

nlohmann::json CurrentAccount::typeSpecificJson() const {
    // Current accounts have no extra fields beyond the base Account fields.
    return nlohmann::json::object();
}

nlohmann::json CurrentAccount::toJson() const {
    return {
        {"account_id",            m_accountID},
        {"user_id",               m_userID},
        {"account_type",          getAccountType()},
        {"balance",               m_balance},
        {"status",                getStatusString()},
        {"opened_date",           m_openedDate},
        {"daily_withdrawn_today", m_dailyWithdrawnToday},
        {"last_withdraw_date",    m_lastWithdrawDate}
    };
}

} // namespace novabanc
