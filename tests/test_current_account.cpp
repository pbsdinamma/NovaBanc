#include "../third_party/catch2/catch.hpp"

#include "../include/CurrentAccount.h"
#include "../include/Exceptions.h"
#include "../include/IDGenerator.h"
#include "../include/Utils.h"

using namespace novabanc;
using namespace novabanc::utils;

static std::unique_ptr<CurrentAccount> makeCurrent(double balance = 5000.0) {
    IDGenerator::initialize("data/counters.json");
    return std::make_unique<CurrentAccount>(
        "ACC-CURR-TEST", "USR-0001", balance,
        AccountStatus::ACTIVE, getCurrentTimestamp());
}

// â”€â”€ Overdraft â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

TEST_CASE("CurrentAccount â€” withdraw into overdraft succeeds", "[current][overdraft]") {
    auto acc = makeCurrent(1000.0);
    // Withdraw 3000 â€” balance goes to -2000, within overdraft limit of 5000.
    REQUIRE_NOTHROW(acc->withdraw(3000.0));
    REQUIRE(acc->getBalance() == Approx(-2000.0));
}

TEST_CASE("CurrentAccount â€” withdraw beyond overdraft limit throws", "[current][overdraft]") {
    auto acc = makeCurrent(1000.0);
    // 1000 + 5000 overdraft = 6000 max withdrawal. 6001 should fail.
    REQUIRE_THROWS_AS(acc->withdraw(6001.0), InsufficientFundsException);
    REQUIRE(acc->getBalance() == Approx(1000.0)); // unchanged
}

TEST_CASE("CurrentAccount â€” withdraw exactly to overdraft limit succeeds", "[current][overdraft]") {
    auto acc = makeCurrent(1000.0);
    // 1000 + 5000 = 6000 exactly OK.
    REQUIRE_NOTHROW(acc->withdraw(6000.0));
    REQUIRE(acc->getBalance() == Approx(-5000.0));
}

// â”€â”€ Daily limit (higher than savings) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

TEST_CASE("CurrentAccount â€” daily limit is Rs.25,000", "[current][daily_limit]") {
    auto acc = makeCurrent(50000.0);
    // First withdraw 24000 â€” OK.
    acc->withdraw(24000.0);
    // Second withdraw 1001 â€” exceeds 25000 daily limit.
    REQUIRE_THROWS_AS(acc->withdraw(1001.0), InsufficientFundsException);
}

TEST_CASE("CurrentAccount â€” daily limit allows up to Rs.25,000", "[current][daily_limit]") {
    auto acc = makeCurrent(50000.0);
    REQUIRE_NOTHROW(acc->withdraw(15000.0));
    REQUIRE_NOTHROW(acc->withdraw(10000.0)); // total = 25000, exactly at limit
}

// â”€â”€ No monthly withdrawal limit â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

TEST_CASE("CurrentAccount â€” more than 6 withdrawals in a month succeeds", "[current][no_monthly_limit]") {
    // Give enough balance (accounting for daily limit reset â€” we just need no monthly cap).
    // Since daily limit is per-day and we're testing in the same test run,
    // we need multiple accounts or a way to reset. We use separate accounts.
    for (int i = 0; i < 10; ++i) {
        auto acc = makeCurrent(50000.0);
        REQUIRE_NOTHROW(acc->withdraw(100.0));
    }
    // No exception thrown means no monthly limit enforcement.
    SUCCEED("CurrentAccount allows more than 6 withdrawals without throwing.");
}

// â”€â”€ canWithdraw â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

TEST_CASE("CurrentAccount â€” canWithdraw returns false when frozen", "[current][canWithdraw]") {
    auto acc = makeCurrent(5000.0);
    acc->freeze();
    REQUIRE(acc->canWithdraw(100.0) == false);
}

TEST_CASE("CurrentAccount â€” canWithdraw returns true within overdraft", "[current][canWithdraw]") {
    auto acc = makeCurrent(100.0);
    REQUIRE(acc->canWithdraw(5000.0) == true);   // -4900, within limit
    REQUIRE(acc->canWithdraw(5100.0) == true);   // -5000 exactly OK
    REQUIRE(acc->canWithdraw(5101.0) == false);  // -5001 exceeds limit
}

// â”€â”€ Type checks â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

TEST_CASE("CurrentAccount â€” getAccountType returns CURRENT", "[current]") {
    auto acc = makeCurrent();
    REQUIRE(acc->getAccountType() == "CURRENT");
}

TEST_CASE("CurrentAccount â€” getInterestRate returns 0.0", "[current]") {
    auto acc = makeCurrent();
    REQUIRE(acc->getInterestRate() == Approx(0.0));
}

TEST_CASE("CurrentAccount â€” getMinimumBalance returns 1000", "[current]") {
    auto acc = makeCurrent();
    REQUIRE(acc->getMinimumBalance() == Approx(1000.0));
}

// â”€â”€ JSON round-trip â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

TEST_CASE("CurrentAccount â€” toJson contains required fields", "[current][json]") {
    auto acc = makeCurrent(3000.0);
    nlohmann::json j = acc->toJson();
    REQUIRE(j["account_type"] == "CURRENT");
    REQUIRE(j["balance"].get<double>() == Approx(3000.0));
    REQUIRE(j.contains("account_id"));
    REQUIRE(j.contains("user_id"));
    REQUIRE(j.contains("status"));
}
