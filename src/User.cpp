#include "User.h"
#include "HashUtils.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace novabanc {

// ── Constructor (new user — hashes password on creation) ──────────────────────

User::User(const std::string& userID,
           const std::string& username,
           const std::string& plaintextPassword,
           const std::string& firstName,
           const std::string& lastName,
           const std::string& email,
           const std::string& phone,
           int                age,
           const std::string& address,
           const std::string& createdAt)
    : m_userID(userID)
    , m_username(username)
    , m_firstName(firstName)
    , m_lastName(lastName)
    , m_email(email)
    , m_phone(phone)
    , m_age(age)
    , m_address(address)
    , m_createdAt(createdAt)
    , m_failedLoginAttempts(0)
    , m_isLocked(false) {
    m_passwordSalt = HashUtils::generateSalt();
    m_passwordHash = HashUtils::hashWithSalt(plaintextPassword, m_passwordSalt);
}

// ── Static factory for loading persisted users ────────────────────────────────

User User::fromStoredHash(const std::string& userID,
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
                          const std::vector<std::string>& accountIDs) {
    User u;
    u.m_userID               = userID;
    u.m_username             = username;
    u.m_passwordHash         = passwordHash;
    u.m_passwordSalt         = passwordSalt;
    u.m_firstName            = firstName;
    u.m_lastName             = lastName;
    u.m_email                = email;
    u.m_phone                = phone;
    u.m_age                  = age;
    u.m_address              = address;
    u.m_createdAt            = createdAt;
    u.m_failedLoginAttempts  = failedLoginAttempts;
    u.m_isLocked             = isLocked;
    u.m_accountIDs           = accountIDs;
    return u;
}

// ── Password management ───────────────────────────────────────────────────────

bool User::verifyPassword(const std::string& input) const {
    return HashUtils::verify(input, m_passwordSalt, m_passwordHash);
}

void User::setPassword(const std::string& newPlaintextPassword) {
    m_passwordSalt = HashUtils::generateSalt();
    m_passwordHash = HashUtils::hashWithSalt(newPlaintextPassword, m_passwordSalt);
}

// ── Login attempt tracking ────────────────────────────────────────────────────

void User::incrementFailedAttempts() { m_failedLoginAttempts++; }
void User::resetFailedAttempts()     { m_failedLoginAttempts = 0; }
void User::lock()                    { m_isLocked = true; }
void User::unlock()                  { m_isLocked = false; m_failedLoginAttempts = 0; }

// ── Account ID management ─────────────────────────────────────────────────────

void User::addAccountID(const std::string& id) {
    m_accountIDs.push_back(id);
}

bool User::removeAccountID(const std::string& id) {
    auto it = std::find(m_accountIDs.begin(), m_accountIDs.end(), id);
    if (it == m_accountIDs.end()) return false;
    m_accountIDs.erase(it);
    return true;
}

// ── JSON serialization ────────────────────────────────────────────────────────

nlohmann::json User::toJson() const {
    nlohmann::json accountIdsJson = nlohmann::json::array();
    for (const auto& id : m_accountIDs) accountIdsJson.push_back(id);

    return {
        {"user_id",               m_userID},
        {"username",              m_username},
        {"password_hash",         m_passwordHash},
        {"password_salt",         m_passwordSalt},
        {"first_name",            m_firstName},
        {"last_name",             m_lastName},
        {"email",                 m_email},
        {"phone",                 m_phone},
        {"age",                   m_age},
        {"address",               m_address},
        {"created_at",            m_createdAt},
        {"failed_login_attempts", m_failedLoginAttempts},
        {"is_locked",             m_isLocked},
        {"account_ids",           accountIdsJson}
    };
}

User User::fromJson(const nlohmann::json& j) {
    std::vector<std::string> accountIDs;
    for (const auto& id : j.at("account_ids")) {
        accountIDs.push_back(id.get<std::string>());
    }

    return User::fromStoredHash(
        j.at("user_id").get<std::string>(),
        j.at("username").get<std::string>(),
        j.at("password_hash").get<std::string>(),
        j.at("password_salt").get<std::string>(),
        j.at("first_name").get<std::string>(),
        j.at("last_name").get<std::string>(),
        j.at("email").get<std::string>(),
        j.at("phone").get<std::string>(),
        j.at("age").get<int>(),
        j.at("address").get<std::string>(),
        j.at("created_at").get<std::string>(),
        j.at("failed_login_attempts").get<int>(),
        j.at("is_locked").get<bool>(),
        accountIDs
    );
}

// ── CSV serialization ─────────────────────────────────────────────────────────

std::string User::csvHeader() {
    return "user_id,username,first_name,last_name,email,phone,age,created_at,is_locked,account_count";
}

std::string User::toCSVRow() const {
    auto csvField = [](const std::string& s) -> std::string {
        if (s.find(',') != std::string::npos) return "\"" + s + "\"";
        return s;
    };

    std::ostringstream oss;
    oss << csvField(m_userID)    << ","
        << csvField(m_username)  << ","
        << csvField(m_firstName) << ","
        << csvField(m_lastName)  << ","
        << csvField(m_email)     << ","
        << csvField(m_phone)     << ","
        << m_age                 << ","
        << csvField(m_createdAt) << ","
        << (m_isLocked ? "YES" : "NO") << ","
        << m_accountIDs.size();
    return oss.str();
}

} // namespace novabanc
