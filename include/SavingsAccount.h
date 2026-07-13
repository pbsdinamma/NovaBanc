#pragma once

#include "Account.h"
#include <string>

// ─────────────────────────────────────────────────────────────────────────────
// SavingsAccount — enforces minimum balance, monthly withdrawal cap, and
// accumulates interest at 3.5% per annum.
// ─────────────────────────────────────────────────────────────────────────────

namespace novabanc {

class SavingsAccount : public Account {
public:
    static constexpr double MIN_BALANCE    = 500.0;
    static constexpr double INTEREST_RATE  = 0.035; // 3.5% per annum

    /// Construct a new SavingsAccount (used at account creation time).
    SavingsAccount(const std::string& accountID,
                   const std::string& userID,
                   double             balance,
                   AccountStatus      status      = AccountStatus::ACTIVE,
                   const std::string& openedDate  = "",
                   double             dailyWithdrawnToday = 0.0,
                   const std::string& lastWithdrawDate    = "",
                   int                monthlyWithdrawals  = 0,
                   const std::string& lastWithdrawMonth   = "");

    // ── Overrides ─────────────────────────────────────────────────────────────
    double      getMinimumBalance() const override { return MIN_BALANCE; }
    double      getInterestRate()   const override { return INTEREST_RATE; }
    std::string getAccountType()    const override { return "SAVINGS"; }

    bool canWithdraw(double amount) const override;

    nlohmann::json toJson() const override;

    // ── SavingsAccount-specific ───────────────────────────────────────────────
    int         getMonthlyWithdrawals()  const { return m_monthlyWithdrawals; }
    std::string getLastWithdrawMonth()   const { return m_lastWithdrawMonth; }

    /// Override withdraw to also track monthly withdrawal count.
    [[nodiscard]] bool withdraw(double amount,
                                const std::string& description = "") override;

    /// Applies (INTEREST_RATE / 12) to the current balance.
    /// Records an INTEREST_CREDIT transaction and returns the interest amount.
    double applyMonthlyInterest();

protected:
    nlohmann::json typeSpecificJson() const override;

private:
    int         m_monthlyWithdrawals;
    std::string m_lastWithdrawMonth;

    void refreshMonthlyCounter();
};

} // namespace novabanc
