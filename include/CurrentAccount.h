#pragma once

#include "Account.h"
#include <string>

// ─────────────────────────────────────────────────────────────────────────────
// CurrentAccount — business-oriented account with overdraft and a higher
// daily withdrawal limit. No monthly withdrawal cap, no interest earned.
// ─────────────────────────────────────────────────────────────────────────────

namespace novabanc {

class CurrentAccount : public Account {
public:
    static constexpr double MIN_BALANCE     = 1000.0;
    static constexpr double OVERDRAFT_LIMIT = 5000.0;
    static constexpr double DAILY_LIMIT     = 25000.0; // shadows Account::DAILY_LIMIT

    /// Construct a new CurrentAccount.
    CurrentAccount(const std::string& accountID,
                   const std::string& userID,
                   double             balance,
                   AccountStatus      status      = AccountStatus::ACTIVE,
                   const std::string& openedDate  = "",
                   double             dailyWithdrawnToday = 0.0,
                   const std::string& lastWithdrawDate    = "");

    // ── Overrides ─────────────────────────────────────────────────────────────
    double      getMinimumBalance() const override { return MIN_BALANCE; }
    double      getInterestRate()   const override { return 0.0; }
    std::string getAccountType()    const override { return "CURRENT"; }

    bool canWithdraw(double amount) const override;

    nlohmann::json toJson() const override;

    // ── Override withdraw for CurrentAccount daily limit ──────────────────────
    [[nodiscard]] bool withdraw(double amount,
                                const std::string& description = "") override;

protected:
    nlohmann::json typeSpecificJson() const override;
};

} // namespace novabanc
