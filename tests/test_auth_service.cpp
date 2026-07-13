#include "../third_party/catch2/catch.hpp"

#include "../include/AuthService.h"
#include "../include/Bank.h"
#include "../include/Exceptions.h"
#include "../include/IDGenerator.h"
#include "../include/Logger.h"
#include "../include/StorageManager.h"
#include "../include/Utils.h"

#include "../include/Filesystem.h"
#include <memory>

using namespace novabanc;
using namespace novabanc::utils;

// Test fixture providing a Bank with a single user.
struct AuthFixture {
    Logger         logger;
    StorageManager storage;
    Bank           bank;
    AuthService    auth;

    AuthFixture()
        : logger("logs/test_auth.log")
        , storage("data")
        , bank("TestBank", storage, logger) {
        IDGenerator::initialize("data/counters.json");
        // Create a test user.
        bank.createUser("alice", "secure1234", "Alice", "Smith",
                        "alice@example.com", "9000000001", 25, "Test City");
    }
};

TEST_CASE("AuthService â€” correct credentials login succeeds", "[auth][login]") {
    AuthFixture f;
    REQUIRE_NOTHROW(f.auth.loginUser("alice", "secure1234", f.bank));
    REQUIRE(f.auth.isLoggedIn() == true);
    REQUIRE(f.auth.isAdmin()    == false);
    REQUIRE(!f.auth.getCurrentUserID().empty());
}

TEST_CASE("AuthService â€” wrong password returns false and increments failed attempts", "[auth][login]") {
    AuthFixture f;
    REQUIRE_THROWS_AS(f.auth.loginUser("alice", "wrongpassword", f.bank),
                      AuthenticationException);
    REQUIRE(f.auth.isLoggedIn() == false);

    User* u = f.bank.findUserByUsername("alice");
    REQUIRE(u != nullptr);
    REQUIRE(u->getFailedAttempts() == 1);
}

TEST_CASE("AuthService â€” unknown username throws AuthenticationException", "[auth][login]") {
    AuthFixture f;
    REQUIRE_THROWS_AS(f.auth.loginUser("nobody", "password1", f.bank),
                      AuthenticationException);
}

TEST_CASE("AuthService â€” three wrong passwords lock the account", "[auth][lockout]") {
    AuthFixture f;
    for (int i = 0; i < 3; ++i) {
        try { f.auth.loginUser("alice", "wrong", f.bank); } catch (...) {}
    }
    User* u = f.bank.findUserByUsername("alice");
    REQUIRE(u != nullptr);
    REQUIRE(u->isLocked() == true);
}

TEST_CASE("AuthService â€” locked account throws on subsequent login", "[auth][lockout]") {
    AuthFixture f;
    User* u = f.bank.findUserByUsername("alice");
    u->lock();
    REQUIRE_THROWS_AS(f.auth.loginUser("alice", "secure1234", f.bank),
                      AuthenticationException);
}

TEST_CASE("AuthService â€” correct admin credentials succeed", "[auth][admin]") {
    AuthService auth;
    REQUIRE_NOTHROW(auth.loginAdmin("admin", "admin123"));
    REQUIRE(auth.isLoggedIn() == true);
    REQUIRE(auth.isAdmin()    == true);
}

TEST_CASE("AuthService â€” wrong admin password throws", "[auth][admin]") {
    AuthService auth;
    REQUIRE_THROWS_AS(auth.loginAdmin("admin", "wrongadmin"),
                      AuthenticationException);
    REQUIRE(auth.isLoggedIn() == false);
}

TEST_CASE("AuthService â€” wrong admin username throws", "[auth][admin]") {
    AuthService auth;
    REQUIRE_THROWS_AS(auth.loginAdmin("root", "admin123"),
                      AuthenticationException);
}

TEST_CASE("AuthService â€” logout clears login state", "[auth][logout]") {
    AuthFixture f;
    f.auth.loginUser("alice", "secure1234", f.bank);
    REQUIRE(f.auth.isLoggedIn() == true);

    f.auth.logout();
    REQUIRE(f.auth.isLoggedIn()          == false);
    REQUIRE(f.auth.isAdmin()             == false);
    REQUIRE(f.auth.getCurrentUserID()    == "");
}
