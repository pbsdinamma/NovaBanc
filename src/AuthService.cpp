#include "AuthService.h"

#include "Bank.h"
#include "Exceptions.h"
#include "HashUtils.h"
#include "User.h"

namespace novabanc {

// ── Admin credentials ─────────────────────────────────────────────────────────
//
// Password: "admin123"
// Salt:     "novabanc_admin_salt_2025"   (fixed, deterministic)
// Hash:     sha256("admin123" + "novabanc_admin_salt_2025")
//
// To verify independently:
//   echo -n "admin123novabanc_admin_salt_2025" | sha256sum
//
// In production this would be read from a config file or secrets manager.
// For this portfolio project, hardcoding is intentional and documented.

const std::string AuthService::ADMIN_USERNAME      = "admin";
const std::string AuthService::ADMIN_PASSWORD_SALT = "novabanc_admin_salt_2025";
const std::string AuthService::ADMIN_PASSWORD_HASH =
    // Pre-computed: sha256("admin123" + "novabanc_admin_salt_2025")
    // This value is computed at startup to handle any platform differences.
    // We store an empty sentinel here and compute lazily.
    "";  // see loginAdmin()

// ── loginUser ─────────────────────────────────────────────────────────────────

bool AuthService::loginUser(const std::string& username,
                            const std::string& password,
                            Bank&              bank) {
    User* user = bank.findUserByUsername(username);
    if (!user) {
        throw AuthenticationException("Invalid username or password.");
    }

    if (user->isLocked()) {
        throw AuthenticationException(
            "Account is locked after " + std::to_string(MAX_ATTEMPTS) +
            " failed login attempts. Please contact the administrator.");
    }

    if (!user->verifyPassword(password)) {
        user->incrementFailedAttempts();

        if (user->getFailedAttempts() >= MAX_ATTEMPTS) {
            user->lock();
            bank.saveAll();
            throw AuthenticationException(
                "Too many failed attempts. Account is now locked.");
        }

        bank.saveAll();
        int remaining = MAX_ATTEMPTS - user->getFailedAttempts();
        throw AuthenticationException(
            "Invalid password. " + std::to_string(remaining) + " attempt(s) remaining.");
    }

    // Success.
    user->resetFailedAttempts();
    bank.saveAll();

    m_loggedInUserID = user->getUserID();
    m_isLoggedIn     = true;
    m_isAdmin        = false;
    return true;
}

// ── loginAdmin ────────────────────────────────────────────────────────────────

bool AuthService::loginAdmin(const std::string& username,
                             const std::string& password) {
    if (username != ADMIN_USERNAME) {
        throw AuthenticationException("Invalid admin credentials.");
    }

    // Compute hash on first call (avoids static initializer order issues).
    std::string expectedHash = HashUtils::hashWithSalt(password, ADMIN_PASSWORD_SALT);

    // The canonical admin password hash (admin123 + salt).
    static const std::string CANONICAL_HASH =
        HashUtils::hashWithSalt("admin123", ADMIN_PASSWORD_SALT);

    if (expectedHash != CANONICAL_HASH) {
        throw AuthenticationException("Invalid admin credentials.");
    }

    m_loggedInUserID = "";
    m_isLoggedIn     = true;
    m_isAdmin        = true;
    return true;
}

// ── logout ────────────────────────────────────────────────────────────────────

void AuthService::logout() {
    m_loggedInUserID = "";
    m_isLoggedIn     = false;
    m_isAdmin        = false;
}

} // namespace novabanc
