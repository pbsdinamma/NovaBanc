#include "Bank.h"

#include <algorithm>
#include <stdexcept>

#include "CurrentAccount.h"
#include "Exceptions.h"
#include "IDGenerator.h"
#include "InputValidator.h"
#include "SavingsAccount.h"
#include "Utils.h"

namespace novabanc {

//Constructor

Bank::Bank(const std::string& bankName,
           StorageManager&    storage,
           Logger&            logger)
    : m_bankName(bankName)
    , m_storage(storage)
    , m_logger(logger) {}

//Persistence

bool Bank::saveAll() {
    return m_storage.saveAll(*this);
}

bool Bank::loadAll() {
    return m_storage.loadAll(*this);
}

//User management

User& Bank::createUser(const std::string& username,
                       const std::string& password,
                       const std::string& firstName,
                       const std::string& lastName,
                       const std::string& email,
                       const std::string& phone,
                       int                age,
                       const std::string& address) {
    // Validation.
    if (m_usernameIndex.count(username)) {
        throw DuplicateUserException("Username '" + username + "' is already taken.");
    }
    if (!InputValidator::isValidEmail(email))
        throw InvalidInputException("Invalid email address.");
    if (!InputValidator::isValidPhone(phone))
        throw InvalidInputException("Phone must be exactly 10 digits.");
    if (!InputValidator::isValidName(firstName))
        throw InvalidInputException("Invalid first name (2-50 letters only).");
    if (!InputValidator::isValidName(lastName))
        throw InvalidInputException("Invalid last name (2-50 letters only).");
    if (!InputValidator::isValidPassword(password))
        throw InvalidInputException("Password must be at least 8 characters and contain a digit.");
    if (!InputValidator::isValidAge(age))
        throw InvalidInputException("Age must be between 18 and 100.");

    std::string userID = IDGenerator::generateUserID();
    std::string ts     = utils::getCurrentTimestamp();

    User newUser(userID, username, password, firstName, lastName,
                 email, phone, age, address, ts);

    m_usernameIndex[username] = userID;
    m_users.emplace(userID, std::move(newUser));
    auto it = m_users.find(userID);
    m_logger.info("User created: " + userID + " (" + username + ")");
    return it->second;
}

bool Bank::deleteUser(const std::string& userID) {
    User* user = findUserByID(userID);
    if (!user) throw AccountNotFoundException("User not found: " + userID);

    // Only delete if no open accounts.
    for (const auto& accID : user->getAccountIDs()) {
        auto it = m_accounts.find(accID);
        if (it != m_accounts.end() && !it->second->isClosed()) {
            return false; // Has open accounts — cannot delete.
        }
    }

    m_usernameIndex.erase(user->getUsername());
    m_users.erase(userID);
    m_logger.info("User deleted: " + userID);
    return true;
}

User* Bank::findUserByID(const std::string& userID) {
    auto it = m_users.find(userID);
    return (it != m_users.end()) ? &it->second : nullptr;
}

User* Bank::findUserByUsername(const std::string& username) {
    auto idIt = m_usernameIndex.find(username);
    if (idIt == m_usernameIndex.end()) return nullptr;
    return findUserByID(idIt->second);
}

std::vector<User*> Bank::searchUsers(const std::string& query) {
    std::string q = query;
    std::transform(q.begin(), q.end(), q.begin(),
                   [](char c) { return static_cast<char>(std::tolower(c)); });

    std::vector<User*> results;
    for (auto& kv : m_users) {
        std::string name  = kv.second.getFullName();
        std::string email = kv.second.getEmail();
        std::transform(name.begin(), name.end(), name.begin(),
                       [](char c) { return static_cast<char>(std::tolower(c)); });
        std::transform(email.begin(), email.end(), email.begin(),
                       [](char c) { return static_cast<char>(std::tolower(c)); });
        if (name.find(q) != std::string::npos || email.find(q) != std::string::npos) {
            results.push_back(&kv.second);
        }
    }
    return results;
}

std::vector<User*> Bank::getAllUsers() {
    std::vector<User*> result;
    result.reserve(m_users.size());
    for (auto& kv : m_users) result.push_back(&kv.second);
    return result;
}

bool Bank::updateUserEmail(const std::string& userID, const std::string& email) {
    if (!InputValidator::isValidEmail(email))
        throw InvalidInputException("Invalid email address.");
    User* user = findUserByID(userID);
    if (!user) return false;
    user->setEmail(email);
    return true;
}

bool Bank::updateUserPhone(const std::string& userID, const std::string& phone) {
    if (!InputValidator::isValidPhone(phone))
        throw InvalidInputException("Phone must be exactly 10 digits.");
    User* user = findUserByID(userID);
    if (!user) return false;
    user->setPhone(phone);
    return true;
}

// ── Account management ────────────────────────────────────────────────────────

std::shared_ptr<Account> Bank::createSavingsAccount(const std::string& userID,
                                                     double initialDeposit) {
    User* user = findUserByID(userID);
    if (!user) throw AccountNotFoundException("User not found: " + userID);
    if (initialDeposit < SavingsAccount::MIN_BALANCE) {
        throw InvalidInputException(
            "Savings account requires a minimum initial deposit of Rs.500.00.");
    }

    std::string accountID = IDGenerator::generateAccountID();
    std::string ts        = utils::getCurrentTimestamp();

    auto acc = std::make_shared<SavingsAccount>(
        accountID, userID, 0.0, AccountStatus::ACTIVE, ts);
    acc->deposit(initialDeposit, "Initial deposit");

    m_accounts[accountID] = acc;
    user->addAccountID(accountID);
    m_logger.info("Savings account created: " + accountID + " for user " + userID);
    return acc;
}

std::shared_ptr<Account> Bank::createCurrentAccount(const std::string& userID,
                                                      double initialDeposit) {
    User* user = findUserByID(userID);
    if (!user) throw AccountNotFoundException("User not found: " + userID);
    if (initialDeposit < CurrentAccount::MIN_BALANCE) {
        throw InvalidInputException(
            "Current account requires a minimum initial deposit of Rs.1,000.00.");
    }

    std::string accountID = IDGenerator::generateAccountID();
    std::string ts        = utils::getCurrentTimestamp();

    auto acc = std::make_shared<CurrentAccount>(
        accountID, userID, 0.0, AccountStatus::ACTIVE, ts);
    acc->deposit(initialDeposit, "Initial deposit");

    m_accounts[accountID] = acc;
    user->addAccountID(accountID);
    m_logger.info("Current account created: " + accountID + " for user " + userID);
    return acc;
}

bool Bank::closeAccount(const std::string& accountID) {
    Account* acc = findAccountByID(accountID);
    if (!acc) throw AccountNotFoundException("Account not found: " + accountID);
    bool closed = acc->close();
    if (closed) m_logger.info("Account closed: " + accountID);
    return closed;
}

bool Bank::freezeAccount(const std::string& accountID) {
    Account* acc = findAccountByID(accountID);
    if (!acc) throw AccountNotFoundException("Account not found: " + accountID);
    acc->freeze();
    m_logger.info("Account frozen: " + accountID);
    return true;
}

bool Bank::unfreezeAccount(const std::string& accountID) {
    Account* acc = findAccountByID(accountID);
    if (!acc) throw AccountNotFoundException("Account not found: " + accountID);
    acc->unfreeze();
    m_logger.info("Account unfrozen: " + accountID);
    return true;
}

Account* Bank::findAccountByID(const std::string& accountID) {
    auto it = m_accounts.find(accountID);
    return (it != m_accounts.end()) ? it->second.get() : nullptr;
}

std::vector<Account*> Bank::getAccountsByUserID(const std::string& userID) {
    std::vector<Account*> result;
    for (auto& kv : m_accounts) {
        if (kv.second->getUserID() == userID) result.push_back(kv.second.get());
    }
    return result;
}

std::vector<Account*> Bank::getAllAccounts() {
    std::vector<Account*> result;
    result.reserve(m_accounts.size());
    for (auto& kv : m_accounts) result.push_back(kv.second.get());
    return result;
}

// ── Financial operations ──────────────────────────────────────────────────────

bool Bank::deposit(const std::string& accountID,
                   double amount,
                   const std::string& description) {
    Account* acc = findAccountByID(accountID);
    if (!acc) throw AccountNotFoundException("Account not found: " + accountID);
    bool ok = acc->deposit(amount, description);
    if (ok) {
        m_logger.info("Deposit to " + accountID);
        saveAll();
    }
    return ok;
}

bool Bank::withdraw(const std::string& accountID,
                    double amount,
                    const std::string& description) {
    Account* acc = findAccountByID(accountID);
    if (!acc) throw AccountNotFoundException("Account not found: " + accountID);
    bool ok = acc->withdraw(amount, description);
    if (ok) {
        m_logger.info("Withdrawal from " + accountID);
        saveAll();
    }
    return ok;
}

bool Bank::transfer(const std::string& fromID,
                    const std::string& toID,
                    double amount) {
    if (fromID == toID)
        throw InvalidInputException("Cannot transfer to the same account.");

    Account* from = findAccountByID(fromID);
    if (!from) throw AccountNotFoundException("Source account not found: " + fromID);

    Account* to = findAccountByID(toID);
    if (!to)  throw AccountNotFoundException("Destination account not found: " + toID);

    if (to->isClosed())
        throw InvalidInputException("Cannot transfer to a closed account.");

    // Debit source.
    from->withdraw(amount, "Transfer to " + toID);

    // Credit destination. If this throws, reverse the debit.
    try {
        to->deposit(amount, "Transfer from " + fromID);
    } catch (...) {
        from->deposit(amount, "Transfer reversal from " + toID);
        m_logger.error("Transfer credit failed; debit reversed. From=" + fromID + " To=" + toID);
        throw;
    }

    m_logger.info("Transfer from " + fromID + " to " + toID);
    saveAll();
    return true;
}

std::vector<Transaction> Bank::getTransactionHistory(const std::string& accountID) const {
    auto it = m_accounts.find(accountID);
    if (it == m_accounts.end())
        throw AccountNotFoundException("Account not found: " + accountID);
    return it->second->getTransactions();
}

// ── Admin operations ──────────────────────────────────────────────────────────

void Bank::applyMonthlyInterestAll() {
    int count = 0;
    for (auto& kv : m_accounts) {
        if (kv.second->getAccountType() == "SAVINGS" && kv.second->isActive()) {
            SavingsAccount* savings = dynamic_cast<SavingsAccount*>(kv.second.get());
            if (savings) {
                double interest = savings->applyMonthlyInterest();
                m_logger.info("Interest applied to " + kv.first + ": " + std::to_string(interest));
                count++;
            }
        }
    }
    m_logger.info("Monthly interest applied to " + std::to_string(count) + " accounts.");
    saveAll();
}

BankStats Bank::getStats() const {
    BankStats stats;
    stats.totalUsers        = static_cast<int>(m_users.size());
    stats.lockedUsers       = 0;
    stats.totalAccounts     = static_cast<int>(m_accounts.size());
    stats.activeAccounts    = 0;
    stats.frozenAccounts    = 0;
    stats.savingsAccounts   = 0;
    stats.currentAccounts   = 0;
    stats.totalBalance      = 0.0;
    stats.totalTransactions = 0;

    for (const auto& kv : m_users) {
        if (kv.second.isLocked()) stats.lockedUsers++;
    }
    for (const auto& kv : m_accounts) {
        const auto& acc = kv.second;
        if (acc->isActive())                      stats.activeAccounts++;
        if (acc->isFrozen())                      stats.frozenAccounts++;
        if (acc->getAccountType() == "SAVINGS")   stats.savingsAccounts++;
        if (acc->getAccountType() == "CURRENT")   stats.currentAccounts++;
        stats.totalBalance      += acc->getBalance();
        stats.totalTransactions += static_cast<int>(acc->getTransactions().size());
    }
    return stats;
}

} // namespace novabanc
