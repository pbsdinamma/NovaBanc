#include "../third_party/catch2/catch.hpp"

#include "../include/User.h"
#include "../include/IDGenerator.h"
#include "../include/Utils.h"

using namespace novabanc;
using namespace novabanc::utils;

// Helper â€” create a fresh test user.
static User makeTestUser() {
    return User("USR-TEST-0001", "john_doe", "password1",
                "John", "Doe", "john@example.com", "9876543210",
                28, "123 Main St", getCurrentTimestamp());
}

TEST_CASE("User â€” construction stores correct fields", "[user]") {
    User u = makeTestUser();
    REQUIRE(u.getUserID()   == "USR-TEST-0001");
    REQUIRE(u.getUsername() == "john_doe");
    REQUIRE(u.getFirstName()== "John");
    REQUIRE(u.getLastName() == "Doe");
    REQUIRE(u.getEmail()    == "john@example.com");
    REQUIRE(u.getPhone()    == "9876543210");
    REQUIRE(u.getAge()      == 28);
    REQUIRE(u.getFullName() == "John Doe");
    REQUIRE(u.isLocked()    == false);
    REQUIRE(u.getFailedAttempts() == 0);
    REQUIRE(u.getAccountIDs().empty());
}

TEST_CASE("User â€” correct password verifies true", "[user]") {
    User u = makeTestUser();
    REQUIRE(u.verifyPassword("password1") == true);
}

TEST_CASE("User â€” wrong password verifies false", "[user]") {
    User u = makeTestUser();
    REQUIRE(u.verifyPassword("wrongpassword") == false);
    REQUIRE(u.verifyPassword("Password1") == false);   // case-sensitive
    REQUIRE(u.verifyPassword("") == false);
}

TEST_CASE("User â€” account lockout after 3 failed attempts", "[user]") {
    User u = makeTestUser();
    REQUIRE(u.isLocked() == false);

    u.incrementFailedAttempts();
    REQUIRE(u.getFailedAttempts() == 1);
    REQUIRE(u.isLocked() == false);

    u.incrementFailedAttempts();
    REQUIRE(u.getFailedAttempts() == 2);
    REQUIRE(u.isLocked() == false);

    u.incrementFailedAttempts();
    u.lock();   // AuthService calls lock() when attempts >= MAX_ATTEMPTS
    REQUIRE(u.isLocked() == true);
}

TEST_CASE("User â€” unlock resets failed attempts", "[user]") {
    User u = makeTestUser();
    u.incrementFailedAttempts();
    u.incrementFailedAttempts();
    u.lock();
    REQUIRE(u.isLocked() == true);

    u.unlock();
    REQUIRE(u.isLocked() == false);
    REQUIRE(u.getFailedAttempts() == 0);
}

TEST_CASE("User â€” resetFailedAttempts works", "[user]") {
    User u = makeTestUser();
    u.incrementFailedAttempts();
    u.incrementFailedAttempts();
    u.resetFailedAttempts();
    REQUIRE(u.getFailedAttempts() == 0);
}

TEST_CASE("User â€” setPassword changes hash, old password fails", "[user]") {
    User u = makeTestUser();
    REQUIRE(u.verifyPassword("password1") == true);
    u.setPassword("newpassword2");
    REQUIRE(u.verifyPassword("password1") == false);
    REQUIRE(u.verifyPassword("newpassword2") == true);
}

TEST_CASE("User â€” addAccountID / removeAccountID", "[user]") {
    User u = makeTestUser();
    REQUIRE(u.getAccountIDs().empty());

    u.addAccountID("ACC-001");
    u.addAccountID("ACC-002");
    REQUIRE(u.getAccountIDs().size() == 2);

    REQUIRE(u.removeAccountID("ACC-001") == true);
    REQUIRE(u.getAccountIDs().size() == 1);
    REQUIRE(u.getAccountIDs()[0] == "ACC-002");

    REQUIRE(u.removeAccountID("ACC-999") == false); // not found
}

TEST_CASE("User â€” JSON round-trip produces identical data", "[user]") {
    User original = makeTestUser();
    original.addAccountID("ACC-001");
    original.incrementFailedAttempts();

    nlohmann::json j = original.toJson();
    User restored    = User::fromJson(j);

    REQUIRE(restored.getUserID()       == original.getUserID());
    REQUIRE(restored.getUsername()     == original.getUsername());
    REQUIRE(restored.getPasswordHash() == original.getPasswordHash());
    REQUIRE(restored.getPasswordSalt() == original.getPasswordSalt());
    REQUIRE(restored.getFirstName()    == original.getFirstName());
    REQUIRE(restored.getLastName()     == original.getLastName());
    REQUIRE(restored.getEmail()        == original.getEmail());
    REQUIRE(restored.getPhone()        == original.getPhone());
    REQUIRE(restored.getAge()          == original.getAge());
    REQUIRE(restored.getAddress()      == original.getAddress());
    REQUIRE(restored.isLocked()        == original.isLocked());
    REQUIRE(restored.getFailedAttempts()  == original.getFailedAttempts());
    REQUIRE(restored.getAccountIDs()      == original.getAccountIDs());

    // Password still verifies correctly after round-trip.
    REQUIRE(restored.verifyPassword("password1") == true);
}

TEST_CASE("User â€” CSV row contains correct field count", "[user]") {
    User u = makeTestUser();
    u.addAccountID("ACC-001");

    std::string header = User::csvHeader();
    std::string row    = u.toCSVRow();

    // Count commas in header vs. row (same number of fields).
    auto countCommas = [](const std::string& s) {
        return std::count(s.begin(), s.end(), ',');
    };
    REQUIRE(countCommas(header) == countCommas(row));

    // Row contains key identifiers.
    REQUIRE(row.find("USR-TEST-0001") != std::string::npos);
    REQUIRE(row.find("john_doe")      != std::string::npos);
    REQUIRE(row.find("john@example.com") != std::string::npos);
}
