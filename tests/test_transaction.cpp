#include "../third_party/catch2/catch.hpp"

#include "../include/Transaction.h"
#include "../include/Utils.h"

using namespace novabanc;
using namespace novabanc::utils;

static Transaction makeDeposit() {
    return Transaction("TXN-20250601100000-0001",
                       "ACC-TEST-0001",
                       TransactionType::DEPOSIT,
                       5000.0, 5000.0,
                       "2025-06-01T10:00:00",
                       "Initial deposit");
}

TEST_CASE("Transaction â€” getters return correct values", "[transaction]") {
    auto txn = makeDeposit();
    REQUIRE(txn.getID()          == "TXN-20250601100000-0001");
    REQUIRE(txn.getAccountID()   == "ACC-TEST-0001");
    REQUIRE(txn.getType()        == TransactionType::DEPOSIT);
    REQUIRE(txn.getAmount()      == Approx(5000.0));
    REQUIRE(txn.getBalanceAfter()== Approx(5000.0));
    REQUIRE(txn.getTimestamp()   == "2025-06-01T10:00:00");
    REQUIRE(txn.getDescription() == "Initial deposit");
}

TEST_CASE("Transaction â€” getTypeString returns human-readable names", "[transaction]") {
    auto check = [](TransactionType t, const std::string& expected) {
        Transaction txn("ID", "ACC", t, 100.0, 100.0, "2025-01-01T00:00:00", "desc");
        REQUIRE(txn.getTypeString() == expected);
    };
    check(TransactionType::DEPOSIT,         "Deposit");
    check(TransactionType::WITHDRAWAL,      "Withdrawal");
    check(TransactionType::TRANSFER_DEBIT,  "Transfer (Debit)");
    check(TransactionType::TRANSFER_CREDIT, "Transfer (Credit)");
    check(TransactionType::INTEREST_CREDIT, "Interest Credit");
}

TEST_CASE("Transaction â€” JSON round-trip preserves all fields", "[transaction][json]") {
    auto original = makeDeposit();
    nlohmann::json j     = original.toJson();
    Transaction restored = Transaction::fromJson(j);

    REQUIRE(restored.getID()           == original.getID());
    REQUIRE(restored.getAccountID()    == original.getAccountID());
    REQUIRE(restored.getType()         == original.getType());
    REQUIRE(restored.getAmount()       == Approx(original.getAmount()));
    REQUIRE(restored.getBalanceAfter() == Approx(original.getBalanceAfter()));
    REQUIRE(restored.getTimestamp()    == original.getTimestamp());
    REQUIRE(restored.getDescription()  == original.getDescription());
}

TEST_CASE("Transaction â€” JSON round-trip works for all transaction types", "[transaction][json]") {
    std::vector<TransactionType> types = {
        TransactionType::DEPOSIT, TransactionType::WITHDRAWAL,
        TransactionType::TRANSFER_DEBIT, TransactionType::TRANSFER_CREDIT,
        TransactionType::INTEREST_CREDIT
    };
    for (auto t : types) {
        Transaction original("ID-" + std::to_string(static_cast<int>(t)),
                             "ACC-X", t, 100.0, 1100.0, "2025-01-01T00:00:00", "test");
        Transaction restored = Transaction::fromJson(original.toJson());
        REQUIRE(restored.getType() == t);
    }
}

TEST_CASE("Transaction â€” CSV row field count matches header", "[transaction][csv]") {
    auto txn = makeDeposit();
    std::string header = Transaction::csvHeader();
    std::string row    = txn.toCSVRow();

    auto countCommas = [](const std::string& s) {
        return std::count(s.begin(), s.end(), ',');
    };
    REQUIRE(countCommas(header) == countCommas(row));
}

TEST_CASE("Transaction â€” CSV row contains key values", "[transaction][csv]") {
    auto txn = makeDeposit();
    std::string row = txn.toCSVRow();
    REQUIRE(row.find("TXN-20250601100000-0001") != std::string::npos);
    REQUIRE(row.find("ACC-TEST-0001")           != std::string::npos);
    REQUIRE(row.find("DEPOSIT")                 != std::string::npos);
}

TEST_CASE("Transaction â€” CSV description with comma is quoted", "[transaction][csv]") {
    Transaction txn("ID", "ACC", TransactionType::DEPOSIT,
                    100.0, 100.0, "2025-01-01T00:00:00",
                    "Transfer from ACC-001, savings");
    std::string row = txn.toCSVRow();
    // The description field should be wrapped in quotes.
    REQUIRE(row.find('"') != std::string::npos);
}
