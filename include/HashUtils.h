#pragma once

#include <string>

// ─────────────────────────────────────────────────────────────────────────────
// HashUtils — SHA-256 hashing and salt generation.
//
// Passwords are never stored in plaintext. On User creation a random 16-byte
// salt is generated, and the password is hashed as SHA-256(password + salt).
// Only the hex-encoded hash and salt are persisted.
// ─────────────────────────────────────────────────────────────────────────────

namespace novabanc {

class HashUtils {
public:
    // No instances — pure static utility class.
    HashUtils() = delete;

    /// Returns the SHA-256 digest of `input` as a lowercase hex string (64 chars).
    static std::string sha256(const std::string& input);

    /// Generates 16 cryptographically-adequate random bytes returned as a 32-char hex string.
    static std::string generateSalt();

    /// Returns sha256(input + salt).
    static std::string hashWithSalt(const std::string& input, const std::string& salt);

    /// Returns true iff sha256(input + salt) == storedHash.
    static bool verify(const std::string& input,
                       const std::string& salt,
                       const std::string& storedHash);
};

} // namespace novabanc
