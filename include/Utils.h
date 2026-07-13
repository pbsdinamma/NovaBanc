#pragma once

#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// Utils — miscellaneous terminal UI helpers and time utilities.
//
// All functions live in namespace novabanc::utils so they don't pollute the
// global namespace. The template printTable is defined here in the header
// because templates must be visible at the call site.
// ─────────────────────────────────────────────────────────────────────────────

namespace novabanc {
namespace utils {

// ── Time helpers ──────────────────────────────────────────────────────────────

/// Returns current local time as "YYYY-MM-DDTHH:MM:SS" (ISO 8601).
std::string getCurrentTimestamp();

/// Returns current local date as "YYYY-MM-DD".
std::string getCurrentDate();

/// Returns current local month as "YYYY-MM".
std::string getCurrentMonth();

// ── Formatting ────────────────────────────────────────────────────────────────

/// Returns the amount formatted as "Rs.X,XXX.XX" (Unicode rupee where supported).
std::string formatAmount(double amount);

// ── Terminal UI ───────────────────────────────────────────────────────────────

/// Clears the terminal screen (platform-aware).
void clearScreen();

/// Prints a decorative box around `title`.
void printHeader(const std::string& title);

/// Prints a horizontal divider line.
void printDivider();

/// Prints a success message with visual emphasis.
void printSuccess(const std::string& message);

/// Prints an error message with visual emphasis.
void printError(const std::string& message);

/// Prints a warning message.
void printWarning(const std::string& message);

/// Prints column-aligned table rows.
/// `headers` and each inner vector must be the same length.
void printTable(const std::vector<std::string>& headers,
                const std::vector<std::vector<std::string>>& rows);

// ── Input helpers ─────────────────────────────────────────────────────────────

/// Reads a password from stdin without echoing characters. Falls back to
/// normal input on platforms where echo-suppression is unavailable.
std::string getHiddenInput();

/// Prompts "Are you sure? [y/N]: " and returns true only on 'y' or 'Y'.
bool confirmAction(const std::string& prompt);

/// Prints `prompt`, reads a line, returns trimmed string.
std::string promptString(const std::string& prompt);

/// Loops until the user enters a valid double.
double promptDouble(const std::string& prompt);

/// Loops until the user enters a valid int.
int promptInt(const std::string& prompt);

} // namespace utils
} // namespace novabanc
