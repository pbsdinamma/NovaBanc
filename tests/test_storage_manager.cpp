#include "../third_party/catch2/catch.hpp"

#include "../include/Bank.h"
#include "../include/IDGenerator.h"
#include "../include/Logger.h"
#include "../include/SavingsAccount.h"
#include "../include/StorageManager.h"
#include "../include/User.h"
#include "../include/Utils.h"
#include "../include/Filesystem.h"

#include <map>
#include <memory>
#include <string>

using namespace novabanc;
using namespace novabanc::utils;

static const std::string TEST_DATA_DIR = "data/test_storage";

struct StorageFixture {
    Logger         logger;
    StorageManager storage;
    Bank           bank;

    StorageFixture()
        : logger("logs/test_storage.log")
        , storage(TEST_DATA_DIR)
        , bank("TestBank", storage, logger) {
        IDGenerator::initialize(TEST_DATA_DIR + "/counters.json");
    }

    ~StorageFixture() {
        // Clean up test data directory.
        std::error_code ec;
        fs::remove_all(TEST_DATA_DIR, ec);
    }
};

// â”€â”€ Users â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

TEST_CASE("StorageManager â€” save and load users round-trip", "[storage][users]") {
    StorageFixture f;

    f.bank.createUser("bob", "password1", "Bob", "Jones",
                      "bob@test.com", "9001234567", 30, "Test St");
    f.bank.saveAll();

    // Load into fresh maps.
    std::map<std::string, User>   freshUsers;
    std::map<std::string, std::string> freshIndex;
    REQUIRE(f.storage.loadUsers(freshUsers, freshIndex) == true);

    REQUIRE(freshUsers.size() == 1);
    auto it = freshUsers.begin();
    REQUIRE(it->second.getUsername()   == "bob");
    REQUIRE(it->second.getFirstName()  == "Bob");
    REQUIRE(it->second.getEmail()      == "bob@test.com");
    REQUIRE(it->second.verifyPassword("password1") == true);
}

// â”€â”€ Accounts â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

TEST_CASE("StorageManager â€” save and load accounts with correct subtype", "[storage][accounts]") {
    StorageFixture f;

    f.bank.createUser("carol", "password1", "Carol", "White",
                      "carol@test.com", "9009876543", 28, "Address");
    User* user = f.bank.findUserByUsername("carol");
    REQUIRE(user != nullptr);

    f.bank.createSavingsAccount(user->getUserID(), 1000.0);
    f.bank.createCurrentAccount(user->getUserID(), 2000.0);
    f.bank.saveAll();

    std::map<std::string, std::shared_ptr<Account>> freshAccounts;
    REQUIRE(f.storage.loadAccounts(freshAccounts) == true);
    REQUIRE(freshAccounts.size() == 2);

    bool hasSavings = false, hasCurrent = false;
    for (const auto& pair : freshAccounts) {
        auto acc = pair.second;
        if (acc->getAccountType() == "SAVINGS") hasSavings = true;
        if (acc->getAccountType() == "CURRENT") hasCurrent = true;
    }
    REQUIRE(hasSavings == true);
    REQUIRE(hasCurrent == true);
}

// â”€â”€ Transactions â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

TEST_CASE("StorageManager â€” transactions saved and loaded on correct account", "[storage][transactions]") {
    StorageFixture f;

    f.bank.createUser("dave", "password1", "Dave", "Brown",
                      "dave@test.com", "9001112222", 35, "Address");
    User* user = f.bank.findUserByUsername("dave");
    REQUIRE(user != nullptr);

    auto acc = f.bank.createSavingsAccount(user->getUserID(), 5000.0);
    std::string accID = acc->getAccountID();
    f.bank.deposit(accID, 1000.0, "Test deposit");
    f.bank.saveAll();

    // Load accounts and then transactions.
    std::map<std::string, std::shared_ptr<Account>> freshAccounts;
    f.storage.loadAccounts(freshAccounts);
    f.storage.loadTransactions(freshAccounts);

    auto it = freshAccounts.find(accID);
    REQUIRE(it != freshAccounts.end());
    // Initial deposit + second deposit = 2 transactions.
    REQUIRE(it->second->getTransactions().size() == 2);
}

// â”€â”€ Missing file handling â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

TEST_CASE("StorageManager â€” load from missing file doesn't crash", "[storage][missing_file]") {
    fs::remove_all(TEST_DATA_DIR);

    Logger logger2("logs/test_storage2.log");
    StorageManager sm("data/test_storage_missing");
    std::map<std::string, User> users;
    std::map<std::string, std::string> idx;

    REQUIRE_NOTHROW(sm.loadUsers(users, idx));
    REQUIRE(users.empty());

    std::map<std::string, std::shared_ptr<Account>> accounts;
    REQUIRE_NOTHROW(sm.loadAccounts(accounts));
    REQUIRE(accounts.empty());

    // Cleanup.
    std::error_code ec;
    fs::remove_all("data/test_storage_missing", ec);
}
