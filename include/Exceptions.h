#pragma once

#include <stdexcept>
#include <string>

// ─────────────────────────────────────────────────────────────────────────────
// NovaBanc Exception Hierarchy
//
// All exceptions inherit from BankException, which inherits from
// std::runtime_error. The UI layer catches specific subtypes to display
// the right message; it catches BankException as a fallback.
// ─────────────────────────────────────────────────────────────────────────────

namespace novabanc {

/// Base class for all NovaBanc exceptions.
class BankException : public std::runtime_error {
public:
    explicit BankException(const std::string& message)
        : std::runtime_error(message) {}
};

/// Thrown when an account ID is not found in the system.
class AccountNotFoundException : public BankException {
public:
    explicit AccountNotFoundException(const std::string& message)
        : BankException(message) {}
};

/// Thrown when a withdrawal or transfer would violate balance rules.
class InsufficientFundsException : public BankException {
public:
    explicit InsufficientFundsException(const std::string& message)
        : BankException(message) {}
};

/// Thrown when user-supplied input fails validation.
class InvalidInputException : public BankException {
public:
    explicit InvalidInputException(const std::string& message)
        : BankException(message) {}
};

/// Thrown when trying to register a username that already exists.
class DuplicateUserException : public BankException {
public:
    explicit DuplicateUserException(const std::string& message)
        : BankException(message) {}
};

/// Thrown on login failure (wrong credentials, locked account).
class AuthenticationException : public BankException {
public:
    explicit AuthenticationException(const std::string& message)
        : BankException(message) {}
};

/// Thrown when an operation is attempted on a frozen account.
class AccountFrozenException : public BankException {
public:
    explicit AccountFrozenException(const std::string& message)
        : BankException(message) {}
};

/// Thrown on JSON read/write failures.
class StorageException : public BankException {
public:
    explicit StorageException(const std::string& message)
        : BankException(message) {}
};

} // namespace novabanc
