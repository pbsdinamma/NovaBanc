#pragma once

#include <string>

// ─────────────────────────────────────────────────────────────────────────────
// AuthService — manages login state for the current terminal session.
//
// Design intent: kept intentionally simple. A single "who is logged in" flag
// is correct for a single-user terminal application. A production system would
// use session tokens with expiry.
//
// Admin credentials are hardcoded as a SHA-256 hash of "admin123" with a
// fixed salt. In production these would come from a config file or secure
// vault, but hardcoding is honest and appropriate for a portfolio project.
// ─────────────────────────────────────────────────────────────────────────────

namespace novabanc {

class Bank;

class AuthService {
public:
    AuthService() = default;

    static constexpr int MAX_ATTEMPTS = 3;

    /// Attempts to log in the user with the given credentials.
    /// Returns true on success. Throws AuthenticationException on failure.
    bool loginUser(const std::string& username,
                   const std::string& password,
                   Bank&              bank);

    /// Attempts to log in as admin.
    bool loginAdmin(const std::string& username,
                    const std::string& password);

    void logout();

    bool        isLoggedIn()      const { return m_isLoggedIn; }
    bool        isAdmin()         const { return m_isAdmin; }
    std::string getCurrentUserID()const { return m_loggedInUserID; }

private:
    std::string m_loggedInUserID;
    bool        m_isLoggedIn{false};
    bool        m_isAdmin{false};

    // Hardcoded admin credentials.
    // Admin password: "admin123"
    // To regenerate: sha256("admin123" + salt)
    static const std::string ADMIN_USERNAME;
    static const std::string ADMIN_PASSWORD_HASH;
    static const std::string ADMIN_PASSWORD_SALT;
};

} // namespace novabanc
