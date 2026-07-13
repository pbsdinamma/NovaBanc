#include "../third_party/catch2/catch.hpp"

#include "../include/SavingsAccount.h"
#include "../include/CurrentAccount.h"
#include "../include/Exceptions.h"
#include "../include/IDGenerator.h"
#include "../include/Utils.h"

using namespace novabanc;
using namespace novabanc::utils;

// Helper â€” a fresh active savings account with Rs.5,000 balance.
static std::unique_ptr<SavingsAccount> makeSavings(double balance = 5000.0) {
    IDGenerator::initialize("data/counters.json");
    return std::make_unique<SavingsAccount>(
        "ACC-TEST-SAVE", "USR-TEST-0001", balance,
        AccountStatus::ACTIVE, getCurrentTimestamp());
}

static std::unique_ptr<CurrentAccount> makeCurrent(double balance = 5000.0) {
    IDGenerator::initialize("data/counters.json");
    return std::make_unique<CurrentAccount>(
        "ACC-TEST-CURR", "USR-TEST-0001", balance,
        AccountStatus::ACTIVE, getCurrentTimestamp());
}

// â”€â”€ Deposit â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

TEST_CASE("Account â€” deposit positive amount increases balance", "[account][deposit]") {
    auto acc = makeSavings(1000.0);
    acc->deposit(500.0, "test deposit");
    REQUIRE(acc->getBalance() == Approx(1500.0));
}

TEST_CASE("Account â€” deposit records a transaction", "[account][deposit]") {
    auto acc = makeSavings(1000.0);
    REQUIRE(acc->getTransactions().empty());
    acc->deposit(250.0, "test");
    REQUIRE(acc->getTransactions().size() == 1);
    REQUIRE(acc->getTransactions()[0].getType() == TransactionType::DEPOSIT);
    REQUIRE(acc->getTransactions()[0].getAmount() == Approx(250.0));
}

TEST_CASE("Account â€” deposit zero throws InvalidInputException", "[account][deposit]") {
    auto acc = makeSavings(1000.0);
    REQUIRE_THROWS_AS(acc->deposit(0.0), InvalidInputException);
}

TEST_CASE("Account â€” deposit negative throws InvalidInputException", "[account][deposit]") {
    auto acc = makeSavings(1000.0);
    REQUIRE_THROWS_AS(acc->deposit(-100.0), InvalidInputException);
}

TEST_CASE("Account â€” deposit to frozen account throws AccountFrozenException", "[account][deposit]") {
    auto acc = makeSavings(1000.0);
    acc->freeze();
    REQUIRE_THROWS_AS(acc->deposit(500.0), AccountFrozenException);
}

// â”€â”€ Withdraw â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

TEST_CASE("Account â€” withdraw valid amount decreases balance and records transaction", "[account][withdraw]") {
    auto acc = makeSavings(5000.0);
    acc->withdraw(1000.0, "test withdraw");
    REQUIRE(acc->getBalance() == Approx(4000.0));
    REQUIRE(acc->getTransactions().size() == 1);
    REQUIRE(acc->getTransactions()[0].getType() == TransactionType::WITHDRAWAL);
}

TEST_CASE("Account â€” withdraw from frozen account throws AccountFrozenException", "[account][withdraw]") {
    auto acc = makeSavings(5000.0);
    acc->freeze();
    REQUIRE_THROWS_AS(acc->withdraw(100.0), AccountFrozenException);
}

// â”€â”€ freeze / unfreeze â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

TEST_CASE("Account â€” freeze sets status to FROZEN", "[account][status]") {
    auto acc = makeSavings();
    REQUIRE(acc->isActive() == true);
    acc->freeze();
    REQUIRE(acc->isFrozen() == true);
    REQUIRE(acc->isActive() == false);
}

TEST_CASE("Account â€” unfreeze restores ACTIVE status", "[account][status]") {
    auto acc = makeSavings();
    acc->freeze();
    acc->unfreeze();
    REQUIRE(acc->isActive() == true);
    REQUIRE(acc->isFrozen() == false);
}

// â”€â”€ Close â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

TEST_CASE("Account â€” close with non-zero balance returns false", "[account][close]") {
    auto acc = makeSavings(5000.0);
    REQUIRE(acc->close() == false);
    REQUIRE(acc->isClosed() == false);
}

TEST_CASE("Account â€” close with zero balance returns true and sets CLOSED", "[account][close]") {
    // Start with exactly MIN_BALANCE and withdraw it all (current acc has no min)
    auto acc = makeCurrent(0.0);
    // Balance is 0 â€” should close.
    // Current accounts allow 0 balance close.
    REQUIRE(acc->close() == true);
    REQUIRE(acc->isClosed() == true);
}

TEST_CASE("Account â€” cannot deposit into closed account", "[account][close]") {
    auto acc = makeCurrent(0.0);
    acc->close();
    REQUIRE_THROWS_AS(acc->deposit(100.0), InvalidInputException);
}

// â”€â”€ Status string â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

TEST_CASE("Account â€” getStatusString returns correct strings", "[account][status]") {
    auto acc = makeSavings();
    REQUIRE(acc->getStatusString() == "ACTIVE");
    acc->freeze();
    REQUIRE(acc->getStatusString() == "FROZEN");
    auto acc2 = makeCurrent(0.0);
    acc2->close();
    REQUIRE(acc2->getStatusString() == "CLOSED");
}

// â”€â”€ balanceAfter in transaction â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

TEST_CASE("Account â€” transaction records correct balanceAfter", "[account][transaction]") {
    auto acc = makeSavings(1000.0);
    acc->deposit(500.0);
    REQUIRE(acc->getTransactions()[0].getBalanceAfter() == Approx(1500.0));
}
