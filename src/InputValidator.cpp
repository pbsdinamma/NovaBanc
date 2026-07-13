#include "InputValidator.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>

namespace novabanc {

bool InputValidator::isValidEmail(const std::string& email) {
    if (email.empty()) return false;

    // No spaces allowed.
    if (email.find(' ') != std::string::npos) return false;

    size_t atPos = email.find('@');
    if (atPos == std::string::npos || atPos == 0) return false;

    // Must have a '.' after the '@'.
    size_t dotPos = email.find('.', atPos);
    if (dotPos == std::string::npos) return false;

    // Something must follow the final dot.
    if (dotPos >= email.size() - 1) return false;

    return true;
}

bool InputValidator::isValidPhone(const std::string& phone) {
    if (phone.size() != 10) return false;
    return std::all_of(phone.begin(), phone.end(),
                       [](char c) { return std::isdigit(static_cast<unsigned char>(c)); });
}

bool InputValidator::isValidName(const std::string& name) {
    if (name.size() < 2 || name.size() > 50) return false;
    return std::all_of(name.begin(), name.end(), [](char c) {
        return std::isalpha(static_cast<unsigned char>(c)) || c == ' ';
    });
}

bool InputValidator::isValidPassword(const std::string& pw) {
    if (pw.size() < 8) return false;
    return std::any_of(pw.begin(), pw.end(),
                       [](char c) { return std::isdigit(static_cast<unsigned char>(c)); });
}

bool InputValidator::isValidAmount(double amount) {
    if (std::isnan(amount) || std::isinf(amount)) return false;
    return amount > 0.0;
}

bool InputValidator::isValidAge(int age) {
    return age >= 18 && age <= 100;
}

std::string InputValidator::trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

} // namespace novabanc
