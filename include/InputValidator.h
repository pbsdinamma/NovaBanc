#pragma once

#include <string>

// ─────────────────────────────────────────────────────────────────────────────
// InputValidator — stateless validation helpers.
//
// All methods return bool. They never throw. Callers decide whether to throw
// InvalidInputException on a false return. This separation keeps the validator
// testable without needing exception handling in test code.
// ─────────────────────────────────────────────────────────────────────────────

namespace novabanc {

class InputValidator {
public:
    InputValidator() = delete;

    /// Must contain '@' and '.', no spaces.
    static bool isValidEmail(const std::string& email);

    /// Exactly 10 decimal digits.
    static bool isValidPhone(const std::string& phone);

    /// 2–50 characters, letters and spaces only.
    static bool isValidName(const std::string& name);

    /// Minimum 8 characters, at least one digit.
    static bool isValidPassword(const std::string& pw);

    /// Positive, not NaN, not infinite.
    static bool isValidAmount(double amount);

    /// Between 18 and 100 inclusive.
    static bool isValidAge(int age);

    /// Removes leading and trailing whitespace.
    static std::string trim(const std::string& s);
};

} // namespace novabanc
