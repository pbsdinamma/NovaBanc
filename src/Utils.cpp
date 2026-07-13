#include "Utils.h"
#include "InputValidator.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

namespace novabanc {
namespace utils {

// ── Time helpers ──────────────────────────────────────────────────────────────

static std::tm localNow() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_val{};
#ifdef _WIN32
    tm_val = *std::localtime(&t);
#else
    localtime_r(&t, &tm_val);
#endif
    return tm_val;
}

std::string getCurrentTimestamp() {
    std::tm tm_val = localNow();
    std::ostringstream oss;
    oss << std::put_time(&tm_val, "%Y-%m-%dT%H:%M:%S");
    return oss.str();
}

std::string getCurrentDate() {
    std::tm tm_val = localNow();
    std::ostringstream oss;
    oss << std::put_time(&tm_val, "%Y-%m-%d");
    return oss.str();
}

std::string getCurrentMonth() {
    std::tm tm_val = localNow();
    std::ostringstream oss;
    oss << std::put_time(&tm_val, "%Y-%m");
    return oss.str();
}

// ── Formatting ────────────────────────────────────────────────────────────────

std::string formatAmount(double amount) {
    // Build the number with 2 decimal places, then insert commas.
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << std::abs(amount);
    std::string num = oss.str();

    // Find the decimal point and work backwards inserting commas.
    size_t dotPos = num.find('.');
    std::string intPart = num.substr(0, dotPos);
    std::string fracPart = num.substr(dotPos); // includes '.'

    // Insert commas every 3 digits from the right of the integer part.
    if (intPart.size() > 3) {
        int insertAt = static_cast<int>(intPart.size()) - 3;
        while (insertAt > 0) {
            intPart.insert(static_cast<size_t>(insertAt), ",");
            insertAt -= 3;
        }
    }

    std::string result = (amount < 0 ? "-Rs." : "Rs.") + intPart + fracPart;
    return result;
}

// ── Terminal UI ───────────────────────────────────────────────────────────────

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void printHeader(const std::string& title) {
    const int WIDTH = 46;
#ifdef _WIN32
    std::string top    = "+";
    std::string bottom = "+";
    std::string side   = "|";
    std::string hline  = "-";
    std::string tr     = "+";
    std::string br     = "+";
#else
    std::string top    = "\u2554";
    std::string bottom = "\u255a";
    std::string side   = "\u2551";
    std::string hline  = "\u2550";
    std::string tr     = "\u2557";
    std::string br     = "\u255d";
#endif

    for (int i = 0; i < WIDTH - 2; ++i) {
        top    += hline;
        bottom += hline;
    }
    top    += tr;
    bottom += br;

    // Center the title.
    int padding = WIDTH - 2 - static_cast<int>(title.size());
    int leftPad  = padding / 2;
    int rightPad = padding - leftPad;

    std::cout << "\n" << top << "\n";
    std::cout << side
              << std::string(static_cast<size_t>(leftPad), ' ')
              << title
              << std::string(static_cast<size_t>(rightPad), ' ')
              << side << "\n";
    std::cout << bottom << "\n\n";
}

void printDivider() {
#ifdef _WIN32
    std::cout << "  --------------------------------------------------------------------------------\n";
#else
    std::cout << "  \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500"
                 "\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500"
                 "\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500"
                 "\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500"
                 "\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n";
#endif
}

void printSuccess(const std::string& message) {
#ifdef _WIN32
    std::cout << "\n  [SUCCESS] " << message << "\n\n";
#else
    std::cout << "\n  \u2714  " << message << "\n\n";
#endif
}

void printError(const std::string& message) {
#ifdef _WIN32
    std::cout << "\n  [ERROR] " << message << "\n\n";
#else
    std::cout << "\n  \u2716  Error: " << message << "\n\n";
#endif
}

void printWarning(const std::string& message) {
#ifdef _WIN32
    std::cout << "\n  [WARNING] " << message << "\n\n";
#else
    std::cout << "\n  \u26a0  Warning: " << message << "\n\n";
#endif
}

void printTable(const std::vector<std::string>& headers,
                const std::vector<std::vector<std::string>>& rows) {
    if (headers.empty()) return;

    // Compute column widths.
    std::vector<size_t> widths(headers.size(), 0);
    for (size_t c = 0; c < headers.size(); ++c) {
        widths[c] = headers[c].size();
    }
    for (const auto& row : rows) {
        for (size_t c = 0; c < headers.size() && c < row.size(); ++c) {
            widths[c] = std::max(widths[c], row[c].size());
        }
    }

    // Header row.
    std::cout << "\n  ";
    for (size_t c = 0; c < headers.size(); ++c) {
        std::cout << std::left << std::setw(static_cast<int>(widths[c]) + 3) << headers[c];
    }
    std::cout << "\n  ";

    // Divider.
    for (size_t c = 0; c < headers.size(); ++c) {
#ifdef _WIN32
        for (size_t i = 0; i < widths[c] + 2; ++i) std::cout << "-";
#else
        for (size_t i = 0; i < widths[c] + 2; ++i) std::cout << "\u2500";
#endif
        std::cout << " ";
    }
    std::cout << "\n";

    // Data rows.
    for (const auto& row : rows) {
        std::cout << "  ";
        for (size_t c = 0; c < headers.size(); ++c) {
            std::string cell = (c < row.size()) ? row[c] : "";
            std::cout << std::left << std::setw(static_cast<int>(widths[c]) + 3) << cell;
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}

// ── Input helpers ─────────────────────────────────────────────────────────────

std::string getHiddenInput() {
    std::string password;

#ifdef _WIN32
    char ch;
    while ((ch = static_cast<char>(_getch())) != '\r' && ch != '\n') {
        if (ch == '\b') {
            if (!password.empty()) {
                password.pop_back();
                std::cout << "\b \b";
            }
        } else {
            password += ch;
            std::cout << '*';
        }
    }
    std::cout << '\n';
#else
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~static_cast<tcflag_t>(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    char ch;
    while (std::cin.get(ch) && ch != '\n') {
        if (ch == 127 || ch == '\b') {
            if (!password.empty()) {
                password.pop_back();
                std::cout << "\b \b";
            }
        } else {
            password += ch;
            std::cout << '*';
        }
    }
    std::cout << '\n';

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif

    return password;
}

bool confirmAction(const std::string& prompt) {
    std::cout << "\n  " << prompt << " [y/N]: ";
    std::string input;
    std::getline(std::cin, input);
    input = InputValidator::trim(input);
    return (input == "y" || input == "Y");
}

std::string promptString(const std::string& prompt) {
    std::cout << "  " << prompt;
    std::string line;
    std::getline(std::cin, line);
    return InputValidator::trim(line);
}

double promptDouble(const std::string& prompt) {
    while (true) {
        std::cout << "  " << prompt;
        std::string line;
        std::getline(std::cin, line);
        line = InputValidator::trim(line);
        try {
            size_t pos;
            double val = std::stod(line, &pos);
            if (pos == line.size()) return val;
        } catch (...) {}
        printError("Please enter a valid number.");
    }
}

int promptInt(const std::string& prompt) {
    while (true) {
        std::cout << "  " << prompt;
        std::string line;
        std::getline(std::cin, line);
        line = InputValidator::trim(line);
        try {
            size_t pos;
            long val = std::stol(line, &pos);
            if (pos == line.size()) return static_cast<int>(val);
        } catch (...) {}
        printError("Please enter a valid integer.");
    }
}

} // namespace utils
} // namespace novabanc
