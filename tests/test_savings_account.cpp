#include "../third_party/catch2/catch.hpp"

#include "../include/SavingsAccount.h"
#include "../include/Exceptions.h"
#include "../include/IDGenerator.h"
#include "../include/Utils.h"

using namespace novabanc;
using namespace novabanc::utils;

static std::unique_ptr<SavingsAccount> makeSavings(double balance = 5000.0) {
    IDGenerator::initialize("data/counters.json");
    return std::make_unique<SavingsAccount>(
        "ACC-SAVE-TEST", "USR-0001", balance,
        AccountStatus::ACTIVE, getCurrentTimestamp());
}

// â”€â”€ Minimum balance enforcement â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

TEST_CASE("SavingsAccount â€” withdraw keeping minimum balance succeeds", "[savings][withdraw]") {
    auto acc = makeSavings(1000.0);
    // Min balance = 500. Can withdraw 500.
    REQUIRE_NOTHROW(acc->withdraw(500.0));
    REQUIRE(acc->getBalance() == Approx(500.0));
}

TEST_CASE("SavingsAccount â€” withdraw violating minimum balance throws", "[savings][withdraw]") {
    auto acc = makeSavings(600.0);
    // Would leave 100, below 500 min.
    REQUIRE_THROWS_AS(acc->withdraw(200.0), InsufficientFundsException);
    REQUIRE(acc->getBalance() == Approx(600.0)); // unchanged
}

TEST_CASE("SavingsAccount â€” exact minimum balance left is allowed", "[savings][withdraw]") {
    auto acc = makeSavings(1000.0);
    REQUIRE_NOTHROW(acc->withdraw(500.0));
    REQUIRE(acc->getBalance() == Approx(500.0));
}

// â”€â”€ Monthly withdrawal limit â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

TEST_CASE("SavingsAccount â€” first 6 withdrawals in same month succeed", "[savings][monthly_limit]") {
    auto acc = makeSavings(10000.0);
    for (int i = 0; i < 6; ++i) {
        REQUIRE_NOTHROW(acc->withdraw(100.0));
    }
    REQUIRE(acc->getMonthlyWithdrawals() == 6);
}

TEST_CASE("SavingsAccount â€” 7th withdrawal in same month throws", "[savings][monthly_limit]") {
    auto acc = makeSavings(10000.0);
    for (int i = 0; i < 6; ++i) {
        acc->withdraw(100.0);
    }
    REQUIRE_THROWS_AS(acc->withdraw(100.0), InsufficientFundsException);
}

TEST_CASE("SavingsAccount â€” monthly withdrawal counter increments correctly", "[savings][monthly_limit]") {
    auto acc = makeSavings(5000.0);
    REQUIRE(acc->getMonthlyWithdrawals() == 0);
    acc->withdraw(100.0);
    REQUIRE(acc->getMonthlyWithdrawals() == 1);
    acc->withdraw(100.0);
    REQUIRE(acc->getMonthlyWithdrawals() == 2);
}

// â”€â”€ Daily limit â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

TEST_CASE("SavingsAccount â€” daily limit is Rs.10,000", "[savings][daily_limit]") {
    auto acc = makeSavings(20000.0);
    // First withdraw 9000 â€” OK.
    acc->withdraw(9000.0);
    // Second withdraw 1001 â€” exceeds daily limit.
    REQUIRE_THROWS_AS(acc->withdraw(1001.0), InsufficientFundsException);
}

TEST_CASE("SavingsAccount â€” daily limit: exactly at limit is allowed", "[savings][daily_limit]") {
    auto acc = makeSavings(15000.0);
    REQUIRE_NOTHROW(acc->withdraw(5000.0));
    REQUIRE_NOTHROW(acc->withdraw(5000.0)); // total = 10000 = limit
}

// â”€â”€ Monthly interest â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

TEST_CASE("SavingsAccount â€” applyMonthlyInterest increases balance", "[savings][interest]") {
    auto acc = makeSavings(12000.0);
    double interest = acc->applyMonthlyInterest();
    // Expected: 12000 * 0.035 / 12 = 35.0
    REQUIRE(interest == Approx(35.0));
    REQUIRE(acc->getBalance() == Approx(12035.0));
}

TEST_CASE("SavingsAccount â€” applyMonthlyInterest records INTEREST_CREDIT transaction", "[savings][interest]") {
    auto acc = makeSavings(12000.0);
    acc->applyMonthlyInterest();
    const auto& txns = acc->getTransactions();
    REQUIRE(!txns.empty());
    REQUIRE(txns.back().getType() == TransactionType::INTEREST_CREDIT);
    REQUIRE(txns.back().getAmount() == Approx(35.0));
}

// â”€â”€ canWithdraw â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

TEST_CASE("SavingsAccount â€” canWithdraw returns false when frozen", "[savings][canWithdraw]") {
    auto acc = makeSavings(5000.0);
    acc->freeze();
    REQUIRE(acc->canWithdraw(100.0) == false);
}

TEST_CASE("SavingsAccount â€” canWithdraw returns false when monthly limit reached", "[savings][canWithdraw]") {
    // Provide the monthly counter directly via constructor.
    IDGenerator::initialize("data/counters.json");
    SavingsAccount acc("ACC-X", "USR-X", 10000.0, AccountStatus::ACTIVE,
                       getCurrentTimestamp(), 0.0, "", 6, getCurrentMonth());
    REQUIRE(acc.canWithdraw(100.0) == false);
}

// â”€â”€ Type checks â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

TEST_CASE("SavingsAccount â€” getAccountType returns SAVINGS", "[savings]") {
    auto acc = makeSavings();
    REQUIRE(acc->getAccountType() == "SAVINGS");
}

TEST_CASE("SavingsAccount â€” getMinimumBalance returns 500", "[savings]") {
    auto acc = makeSavings();
    REQUIRE(acc->getMinimumBalance() == Approx(500.0));
}

TEST_CASE("SavingsAccount â€” getInterestRate returns 0.035", "[savings]") {
    auto acc = makeSavings();
    REQUIRE(acc->getInterestRate() == Approx(0.035));
}

// â”€â”€ JSON round-trip â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

TEST_CASE("SavingsAccount â€” toJson contains required fields", "[savings][json]") {
    auto acc = makeSavings(7500.0);
    acc->withdraw(200.0);

    nlohmann::json j = acc->toJson();
    REQUIRE(j["account_type"]         == "SAVINGS");
    REQUIRE(j["balance"].get<double>()== Approx(7300.0));
    REQUIRE(j.contains("monthly_withdrawals"));
    REQUIRE(j.contains("last_withdraw_month"));
}
