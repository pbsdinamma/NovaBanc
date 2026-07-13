#include "../third_party/catch2/catch.hpp"

#include "../include/CSVExporter.h"
#include "../include/Filesystem.h"
#include "../include/SavingsAccount.h"
#include "../include/Transaction.h"
#include "../include/User.h"
#include "../include/IDGenerator.h"
#include "../include/Utils.h"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace novabanc;
using namespace novabanc::utils;

static const std::string TEST_REPORTS_DIR = "reports/test_csv/";

// Count lines in a file.
static int lineCount(const std::string& path) {
    std::ifstream file(path);
    int count = 0;
    std::string line;
    while (std::getline(file, line)) count++;
    return count;
}

// Read entire file content.
static std::string readFile(const std::string& path) {
    std::ifstream file(path);
    std::ostringstream oss;
    oss << file.rdbuf();
    return oss.str();
}

TEST_CASE("CSVExporter â€” exportUsers creates file with header and one row per user", "[csv][users]") {
    User u1("USR-001", "alice", "pass1234", "Alice", "Smith",
            "alice@test.com", "9000000001", 25, "City A", getCurrentTimestamp());
    User u2("USR-002", "bob",   "pass1234", "Bob",   "Jones",
            "bob@test.com",   "9000000002", 30, "City B", getCurrentTimestamp());

    std::vector<User*> users = {&u1, &u2};
    std::string path = TEST_REPORTS_DIR + "users_test.csv";
    REQUIRE(CSVExporter::exportUsers(users, path) == true);

    int lines = lineCount(path);
    REQUIRE(lines == 3); // header + 2 data rows

    std::string content = readFile(path);
    REQUIRE(content.find("alice") != std::string::npos);
    REQUIRE(content.find("bob")   != std::string::npos);

    fs::remove(path);
}

TEST_CASE("CSVExporter â€” exportUsers with empty list produces only header", "[csv][users]") {
    std::string path = TEST_REPORTS_DIR + "users_empty.csv";
    REQUIRE(CSVExporter::exportUsers({}, path) == true);
    REQUIRE(lineCount(path) == 1); // header only
    fs::remove(path);
}

TEST_CASE("CSVExporter â€” description with comma is quoted in transactions", "[csv][transactions]") {
    Transaction txn("TXN-001", "ACC-001", TransactionType::DEPOSIT,
                    100.0, 100.0, "2025-06-01T10:00:00",
                    "Payment for rent, June");
    std::string path = TEST_REPORTS_DIR + "txn_comma.csv";
    REQUIRE(CSVExporter::exportTransactions({txn}, path) == true);

    std::string content = readFile(path);
    // Description with comma must be quoted.
    REQUIRE(content.find('"') != std::string::npos);
    fs::remove(path);
}

TEST_CASE("CSVExporter â€” exportTransactions with empty list produces only header", "[csv][transactions]") {
    std::string path = TEST_REPORTS_DIR + "txn_empty.csv";
    REQUIRE(CSVExporter::exportTransactions({}, path) == true);
    REQUIRE(lineCount(path) == 1);
    fs::remove(path);
}

TEST_CASE("CSVExporter â€” exportAccountSummary creates correct file", "[csv][accounts]") {
    IDGenerator::initialize("data/counters.json");
    SavingsAccount acc("ACC-TEST", "USR-001", 5000.0,
                       AccountStatus::ACTIVE, getCurrentTimestamp());

    std::vector<Account*> accounts = {&acc};
    std::string path = TEST_REPORTS_DIR + "accounts_test.csv";
    REQUIRE(CSVExporter::exportAccountSummary(accounts, path) == true);

    std::string content = readFile(path);
    REQUIRE(content.find("ACC-TEST") != std::string::npos);
    REQUIRE(content.find("SAVINGS")  != std::string::npos);
    fs::remove(path);
}

TEST_CASE("CSVExporter â€” exportUserStatement includes account info and transactions", "[csv][statement]") {
    IDGenerator::initialize("data/counters.json");
    SavingsAccount acc("ACC-STMT", "USR-001", 0.0,
                       AccountStatus::ACTIVE, getCurrentTimestamp());
    acc.deposit(5000.0, "Initial deposit");
    acc.deposit(1000.0, "Second deposit");

    std::string path = TEST_REPORTS_DIR + "statement_test.csv";
    REQUIRE(CSVExporter::exportUserStatement(acc, path) == true);

    std::string content = readFile(path);
    REQUIRE(content.find("ACC-STMT")        != std::string::npos);
    REQUIRE(content.find("DEPOSIT")         != std::string::npos);
    REQUIRE(content.find("Initial deposit") != std::string::npos);
    fs::remove(path);
}

TEST_CASE("CSVExporter â€” creates parent directory if it doesn't exist", "[csv][directory]") {
    std::string deepPath = TEST_REPORTS_DIR + "deep/nested/test.csv";
    User u("USR-001", "test", "pass1234", "Test", "User",
           "test@test.com", "9000000000", 20, "Addr", getCurrentTimestamp());
    REQUIRE(CSVExporter::exportUsers({&u}, deepPath) == true);
    REQUIRE(fs::exists(deepPath) == true);

    fs::remove_all(TEST_REPORTS_DIR + "deep");
}
