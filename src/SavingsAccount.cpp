#include "SavingsAccount.h"
#include "Exceptions.h"
#include "IDGenerator.h"
#include "Utils.h"

#include <iomanip>
#include <sstream>

namespace novabanc {

// ── Constructor ───────────────────────────────────────────────────────────────

SavingsAccount::SavingsAccount(const std::string& accountID,
                               const std::string& userID,
                               double             balance,
                               AccountStatus      status,
                               const std::string& openedDate,
                               double             dailyWithdrawnToday,
                               const std::string& lastWithdrawDate,
                               int                monthlyWithdrawals,
                               const std::string& lastWithdrawMonth)
    : Account(accountID, userID, balance, status,
              openedDate, dailyWithdrawnToday, lastWithdrawDate)
    , m_monthlyWithdrawals(monthlyWithdrawals)
    , m_lastWithdrawMonth(lastWithdrawMonth) {}

// ── Monthly counter reset ─────────────────────────────────────────────────────

void SavingsAccount::refreshMonthlyCounter() {
    std::string currentMonth = utils::getCurrentMonth();
    if (m_lastWithdrawMonth != currentMonth) {
        m_monthlyWithdrawals = 0;
        m_lastWithdrawMonth  = currentMonth;
    }
}

// ── canWithdraw ───────────────────────────────────────────────────────────────

bool SavingsAccount::canWithdraw(double amount) const {
    if (!isActive()) return false;
    if (m_balance - amount < MIN_BALANCE) return false;
    if (m_dailyWithdrawnToday + amount > DAILY_LIMIT) return false;
    if (m_monthlyWithdrawals >= 6) return false;
    return true;
}

// ── Override withdraw to track monthly count ──────────────────────────────────

bool SavingsAccount::withdraw(double amount, const std::string& description) {
    // Refresh both daily and monthly counters before checking rules.
    refreshDailyLimit();
    refreshMonthlyCounter();

    if (amount <= 0.0) {
        throw InvalidInputException("Withdrawal amount must be a positive number.");
    }
    if (isFrozen()) {
        throw AccountFrozenException("Account " + m_accountID + " is frozen.");
    }
    if (isClosed()) {
        throw InvalidInputException("Cannot withdraw from a closed account.");
    }
    if (m_monthlyWithdrawals >= 6) {
        throw InsufficientFundsException(
            "Monthly withdrawal limit (6) reached for savings account.");
    }
    if (m_balance - amount < MIN_BALANCE) {
        throw InsufficientFundsException(
            "Withdrawal would violate minimum balance of Rs.500.00.");
    }
    if (m_dailyWithdrawnToday + amount > DAILY_LIMIT) {
        throw InsufficientFundsException(
            "Daily withdrawal limit of Rs.10,000.00 would be exceeded.");
    }

    m_balance              -= amount;
    m_dailyWithdrawnToday  += amount;
    m_monthlyWithdrawals++;

    std::string ts   = utils::getCurrentTimestamp();
    std::string id   = IDGenerator::generateTransactionID();
    std::string desc = description.empty() ? "Withdrawal" : description;

    m_transactions.emplace_back(id, m_accountID,
                                TransactionType::WITHDRAWAL,
                                amount, m_balance, ts, desc);
    return true;
}

// ── Monthly interest ──────────────────────────────────────────────────────────

double SavingsAccount::applyMonthlyInterest() {
    double interest = m_balance * (INTEREST_RATE / 12.0);
    if (interest <= 0.0) return 0.0;

    m_balance += interest;

    std::string ts = utils::getCurrentTimestamp();
    std::string id = IDGenerator::generateTransactionID();

    m_transactions.emplace_back(id, m_accountID,
                                TransactionType::INTEREST_CREDIT,
                                interest, m_balance, ts,
                                "Monthly interest credit");
    return interest;
}

// ── JSON serialization ────────────────────────────────────────────────────────

nlohmann::json SavingsAccount::typeSpecificJson() const {
    return {
        {"monthly_withdrawals",  m_monthlyWithdrawals},
        {"last_withdraw_month",  m_lastWithdrawMonth}
    };
}

nlohmann::json SavingsAccount::toJson() const {
    nlohmann::json j = {
        {"account_id",            m_accountID},
        {"user_id",               m_userID},
        {"account_type",          getAccountType()},
        {"balance",               m_balance},
        {"status",                getStatusString()},
        {"opened_date",           m_openedDate},
        {"daily_withdrawn_today", m_dailyWithdrawnToday},
        {"last_withdraw_date",    m_lastWithdrawDate}
    };

    nlohmann::json specific = typeSpecificJson();
    j.merge_patch(specific);
    return j;
}

} // namespace novabanc
