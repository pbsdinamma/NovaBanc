#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

// ─────────────────────────────────────────────────────────────────────────────
// User — represents a bank customer.
//
// Passwords are never stored in plaintext. The constructor accepts a plaintext
// password and hashes it immediately with a generated salt.  Verification
// re-hashes the input with the stored salt and compares.
// ─────────────────────────────────────────────────────────────────────────────

namespace novabanc {

class User {
public:
    /// Create a new user. Password is hashed on construction.
    User(const std::string& userID,
         const std::string& username,
         const std::string& plaintextPassword,
         const std::string& firstName,
         const std::string& lastName,
         const std::string& email,
         const std::string& phone,
         int                age,
         const std::string& address,
         const std::string& createdAt);

    /// Restore a User from persisted data (password already hashed).
    static User fromStoredHash(const std::string& userID,
                               const std::string& username,
                               const std::string& passwordHash,
                               const std::string& passwordSalt,
                               const std::string& firstName,
                               const std::string& lastName,
                               const std::string& email,
                               const std::string& phone,
                               int                age,
                               const std::string& address,
                               const std::string& createdAt,
                               int                failedLoginAttempts,
                               bool               isLocked,
                               const std::vector<std::string>& accountIDs);

    // ── Getters ───────────────────────────────────────────────────────────────
    const std::string& getUserID()         const { return m_userID; }
    const std::string& getUsername()       const { return m_username; }
    const std::string& getPasswordHash()   const { return m_passwordHash; }
    const std::string& getPasswordSalt()   const { return m_passwordSalt; }
    const std::string& getFirstName()      const { return m_firstName; }
    const std::string& getLastName()       const { return m_lastName; }
    const std::string& getEmail()          const { return m_email; }
    const std::string& getPhone()          const { return m_phone; }
    int                getAge()            const { return m_age; }
    const std::string& getAddress()        const { return m_address; }
    const std::string& getCreatedAt()      const { return m_createdAt; }
    int                getFailedAttempts() const { return m_failedLoginAttempts; }
    bool               isLocked()          const { return m_isLocked; }
    std::string        getFullName()       const { return m_firstName + " " + m_lastName; }

    const std::vector<std::string>& getAccountIDs() const { return m_accountIDs; }

    // ── Password management ───────────────────────────────────────────────────

    /// Returns true iff sha256(input + m_passwordSalt) == m_passwordHash.
    bool verifyPassword(const std::string& input) const;

    /// Generates a new salt and re-hashes the supplied password.
    void setPassword(const std::string& newPlaintextPassword);

    // ── Profile setters ───────────────────────────────────────────────────────
    void setEmail(const std::string& email)   { m_email = email; }
    void setPhone(const std::string& phone)   { m_phone = phone; }
    void setAddress(const std::string& addr)  { m_address = addr; }
    void setAge(int age)                      { m_age = age; }

    // ── Login attempt tracking ────────────────────────────────────────────────
    void incrementFailedAttempts();
    void resetFailedAttempts();
    void lock();
    void unlock();

    // ── Account ID management ─────────────────────────────────────────────────
    void addAccountID(const std::string& id);

    /// Returns false if the ID was not found.
    bool removeAccountID(const std::string& id);

    // ── Serialization ─────────────────────────────────────────────────────────
    nlohmann::json toJson() const;
    static User fromJson(const nlohmann::json& j);

    std::string        toCSVRow() const;
    static std::string csvHeader();

private:
    std::string              m_userID;
    std::string              m_username;
    std::string              m_passwordHash;
    std::string              m_passwordSalt;
    std::string              m_firstName;
    std::string              m_lastName;
    std::string              m_email;
    std::string              m_phone;
    int                      m_age;
    std::string              m_address;
    std::string              m_createdAt;
    int                      m_failedLoginAttempts;
    bool                     m_isLocked;
    std::vector<std::string> m_accountIDs;

    // Private constructor used by fromStoredHash.
    User() = default;
};

} // namespace novabanc
