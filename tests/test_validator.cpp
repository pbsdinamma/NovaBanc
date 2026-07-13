#include "../third_party/catch2/catch.hpp"

#include "../include/InputValidator.h"

using namespace novabanc;

// â”€â”€ Email â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

TEST_CASE("InputValidator â€” valid emails pass", "[validator][email]") {
    REQUIRE(InputValidator::isValidEmail("user@example.com")   == true);
    REQUIRE(InputValidator::isValidEmail("a@b.io")             == true);
    REQUIRE(InputValidator::isValidEmail("first.last@co.in")   == true);
}

TEST_CASE("InputValidator â€” invalid emails fail", "[validator][email]") {
    REQUIRE(InputValidator::isValidEmail("")              == false);
    REQUIRE(InputValidator::isValidEmail("notanemail")   == false);
    REQUIRE(InputValidator::isValidEmail("@example.com") == false);
    REQUIRE(InputValidator::isValidEmail("user@")        == false);
    REQUIRE(InputValidator::isValidEmail("user @ex.com") == false); // space
    REQUIRE(InputValidator::isValidEmail("user@example") == false); // no dot after @
}

// â”€â”€ Phone â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

TEST_CASE("InputValidator â€” exactly 10 digit phone passes", "[validator][phone]") {
    REQUIRE(InputValidator::isValidPhone("9876543210") == true);
    REQUIRE(InputValidator::isValidPhone("0000000000") == true);
}

TEST_CASE("InputValidator â€” invalid phones fail", "[validator][phone]") {
    REQUIRE(InputValidator::isValidPhone("")              == false);
    REQUIRE(InputValidator::isValidPhone("987654321")    == false); // 9 digits
    REQUIRE(InputValidator::isValidPhone("98765432100")  == false); // 11 digits
    REQUIRE(InputValidator::isValidPhone("987654321a")   == false); // letter
    REQUIRE(InputValidator::isValidPhone("+9876543210")  == false); // plus sign
}

// â”€â”€ Name â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

TEST_CASE("InputValidator â€” valid names pass", "[validator][name]") {
    REQUIRE(InputValidator::isValidName("Alice")       == true);
    REQUIRE(InputValidator::isValidName("John Doe")    == true);
    REQUIRE(InputValidator::isValidName("AB")          == true); // exactly 2 chars
}

TEST_CASE("InputValidator â€” invalid names fail", "[validator][name]") {
    REQUIRE(InputValidator::isValidName("")            == false);
    REQUIRE(InputValidator::isValidName("A")           == false); // 1 char
    REQUIRE(InputValidator::isValidName("Alice123")    == false); // digits
    REQUIRE(InputValidator::isValidName("Alice@Name")  == false); // special char
    // 51-character name
    REQUIRE(InputValidator::isValidName(std::string(51, 'A')) == false);
}

// â”€â”€ Password â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

TEST_CASE("InputValidator â€” valid passwords pass", "[validator][password]") {
    REQUIRE(InputValidator::isValidPassword("password1") == true);
    REQUIRE(InputValidator::isValidPassword("12345678")  == true); // all digits, >=8
    REQUIRE(InputValidator::isValidPassword("longpass9") == true);
}

TEST_CASE("InputValidator â€” invalid passwords fail", "[validator][password]") {
    REQUIRE(InputValidator::isValidPassword("short1")    == false); // < 8 chars
    REQUIRE(InputValidator::isValidPassword("nodigits")  == false); // no digit
    REQUIRE(InputValidator::isValidPassword("")          == false);
    REQUIRE(InputValidator::isValidPassword("1234567")   == false); // 7 chars with digit
}

// â”€â”€ Amount â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

TEST_CASE("InputValidator â€” positive amount is valid", "[validator][amount]") {
    REQUIRE(InputValidator::isValidAmount(0.01)    == true);
    REQUIRE(InputValidator::isValidAmount(1000.0)  == true);
    REQUIRE(InputValidator::isValidAmount(0.001)   == true);
}

TEST_CASE("InputValidator â€” zero amount is invalid", "[validator][amount]") {
    REQUIRE(InputValidator::isValidAmount(0.0)     == false);
}

TEST_CASE("InputValidator â€” negative amount is invalid", "[validator][amount]") {
    REQUIRE(InputValidator::isValidAmount(-1.0)    == false);
    REQUIRE(InputValidator::isValidAmount(-0.001)  == false);
}

TEST_CASE("InputValidator â€” NaN and infinity are invalid", "[validator][amount]") {
    REQUIRE(InputValidator::isValidAmount(std::numeric_limits<double>::quiet_NaN()) == false);
    REQUIRE(InputValidator::isValidAmount(std::numeric_limits<double>::infinity())  == false);
}

// â”€â”€ Age â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

TEST_CASE("InputValidator â€” valid ages pass", "[validator][age]") {
    REQUIRE(InputValidator::isValidAge(18)  == true);
    REQUIRE(InputValidator::isValidAge(25)  == true);
    REQUIRE(InputValidator::isValidAge(100) == true);
}

TEST_CASE("InputValidator â€” age below 18 fails", "[validator][age]") {
    REQUIRE(InputValidator::isValidAge(17) == false);
    REQUIRE(InputValidator::isValidAge(0)  == false);
}

TEST_CASE("InputValidator â€” age above 100 fails", "[validator][age]") {
    REQUIRE(InputValidator::isValidAge(101) == false);
    REQUIRE(InputValidator::isValidAge(200) == false);
}

// â”€â”€ Trim â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

TEST_CASE("InputValidator â€” trim removes leading whitespace", "[validator][trim]") {
    REQUIRE(InputValidator::trim("   hello") == "hello");
    REQUIRE(InputValidator::trim("\t\nhello") == "hello");
}

TEST_CASE("InputValidator â€” trim removes trailing whitespace", "[validator][trim]") {
    REQUIRE(InputValidator::trim("hello   ") == "hello");
    REQUIRE(InputValidator::trim("hello\n")  == "hello");
}

TEST_CASE("InputValidator â€” trim leaves middle spaces intact", "[validator][trim]") {
    REQUIRE(InputValidator::trim("  hello world  ") == "hello world");
}

TEST_CASE("InputValidator â€” trim handles all-whitespace string", "[validator][trim]") {
    REQUIRE(InputValidator::trim("   ") == "");
    REQUIRE(InputValidator::trim("")    == "");
}

TEST_CASE("InputValidator â€” trim on already-clean string returns same", "[validator][trim]") {
    REQUIRE(InputValidator::trim("clean") == "clean");
}
