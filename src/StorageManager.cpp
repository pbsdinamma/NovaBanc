#include "StorageManager.h"

#include <fstream>
#include <iostream>

#include "Account.h"
#include "Bank.h"
#include "CurrentAccount.h"
#include "Exceptions.h"
#include "Filesystem.h"
#include "SavingsAccount.h"
#include "Transaction.h"
#include "User.h"
#include <nlohmann/json.hpp>

namespace novabanc {

// ── Constructor ───────────────────────────────────────────────────────────────

StorageManager::StorageManager(const std::string& dataDirectory)
    : m_dataDir(dataDirectory) {
    ensureDataDirectory();
}

// ── Path helpers ──────────────────────────────────────────────────────────────

void StorageManager::ensureDataDirectory() const {
    fs::create_directories(m_dataDir);
}

std::string StorageManager::usersFilePath()        const { return m_dataDir + "/users.json"; }
std::string StorageManager::accountsFilePath()     const { return m_dataDir + "/accounts.json"; }
std::string StorageManager::transactionsFilePath() const { return m_dataDir + "/transactions.json"; }

// ── Save methods ──────────────────────────────────────────────────────────────

bool StorageManager::saveUsers(const std::map<std::string, User>& users) {
    try {
        nlohmann::json j;
        j["users"] = nlohmann::json::array();
        for (const auto& kv : users) {
            j["users"].push_back(kv.second.toJson());
        }
        std::ofstream file(usersFilePath());
        if (!file.is_open()) throw StorageException("Cannot open users.json for writing.");
        file << j.dump(4);
        return true;
    } catch (const StorageException&) {
        throw;
    } catch (const std::exception& e) {
        throw StorageException(std::string("saveUsers failed: ") + e.what());
    }
}

bool StorageManager::saveAccounts(const std::map<std::string, std::shared_ptr<Account>>& accounts) {
    try {
        nlohmann::json j;
        j["accounts"] = nlohmann::json::array();
        for (const auto& kv : accounts) {
            j["accounts"].push_back(kv.second->toJson());
        }
        std::ofstream file(accountsFilePath());
        if (!file.is_open()) throw StorageException("Cannot open accounts.json for writing.");
        file << j.dump(4);
        return true;
    } catch (const StorageException&) {
        throw;
    } catch (const std::exception& e) {
        throw StorageException(std::string("saveAccounts failed: ") + e.what());
    }
}

bool StorageManager::saveTransactions(const std::map<std::string, std::shared_ptr<Account>>& accounts) {
    try {
        nlohmann::json j;
        j["transactions"] = nlohmann::json::array();
        for (const auto& kv : accounts) {
            for (const auto& txn : kv.second->getTransactions()) {
                j["transactions"].push_back(txn.toJson());
            }
        }
        std::ofstream file(transactionsFilePath());
        if (!file.is_open()) throw StorageException("Cannot open transactions.json for writing.");
        file << j.dump(4);
        return true;
    } catch (const StorageException&) {
        throw;
    } catch (const std::exception& e) {
        throw StorageException(std::string("saveTransactions failed: ") + e.what());
    }
}

// ── Load methods ──────────────────────────────────────────────────────────────

bool StorageManager::loadUsers(std::map<std::string, User>& users,
                               std::map<std::string, std::string>& usernameIndex) {
    if (!fs::exists(usersFilePath())) return true; // empty start

    try {
        std::ifstream file(usersFilePath());
        if (!file.is_open()) throw StorageException("Cannot open users.json for reading.");

        nlohmann::json j;
        file >> j;

        for (const auto& userJson : j.at("users")) {
            User u = User::fromJson(userJson);
            usernameIndex[u.getUsername()] = u.getUserID();
            users.emplace(u.getUserID(), std::move(u));
        }
        return true;
    } catch (const StorageException&) {
        throw;
    } catch (const std::exception& e) {
        throw StorageException(std::string("loadUsers failed: ") + e.what());
    }
}

bool StorageManager::loadAccounts(std::map<std::string, std::shared_ptr<Account>>& accounts) {
    if (!fs::exists(accountsFilePath())) return true;

    try {
        std::ifstream file(accountsFilePath());
        if (!file.is_open()) throw StorageException("Cannot open accounts.json for reading.");

        nlohmann::json j;
        file >> j;

        for (const auto& accJson : j.at("accounts")) {
            std::string type      = accJson.at("account_type").get<std::string>();
            std::string accountID = accJson.at("account_id").get<std::string>();
            std::string userID    = accJson.at("user_id").get<std::string>();
            double      balance   = accJson.at("balance").get<double>();

            std::string statusStr = accJson.at("status").get<std::string>();
            AccountStatus status  = AccountStatus::ACTIVE;
            if (statusStr == "FROZEN") status = AccountStatus::FROZEN;
            else if (statusStr == "CLOSED") status = AccountStatus::CLOSED;

            std::string openedDate       = accJson.at("opened_date").get<std::string>();
            double      dailyWithdrawn   = accJson.value("daily_withdrawn_today", 0.0);
            std::string lastWithdrawDate = accJson.value("last_withdraw_date", std::string(""));

            std::shared_ptr<Account> acc;
            if (type == "SAVINGS") {
                int         monthlyWithdrawals = accJson.value("monthly_withdrawals", 0);
                std::string lastMonth          = accJson.value("last_withdraw_month", std::string(""));
                acc = std::make_shared<SavingsAccount>(
                    accountID, userID, balance, status, openedDate,
                    dailyWithdrawn, lastWithdrawDate, monthlyWithdrawals, lastMonth);
            } else if (type == "CURRENT") {
                acc = std::make_shared<CurrentAccount>(
                    accountID, userID, balance, status, openedDate,
                    dailyWithdrawn, lastWithdrawDate);
            } else {
                throw StorageException("Unknown account type: " + type);
            }
            accounts.emplace(accountID, std::move(acc));
        }
        return true;
    } catch (const StorageException&) {
        throw;
    } catch (const std::exception& e) {
        throw StorageException(std::string("loadAccounts failed: ") + e.what());
    }
}

bool StorageManager::loadTransactions(std::map<std::string, std::shared_ptr<Account>>& accounts) {
    if (!fs::exists(transactionsFilePath())) return true;

    try {
        std::ifstream file(transactionsFilePath());
        if (!file.is_open()) throw StorageException("Cannot open transactions.json for reading.");

        nlohmann::json j;
        file >> j;

        for (const auto& txnJson : j.at("transactions")) {
            Transaction txn = Transaction::fromJson(txnJson);
            auto it = accounts.find(txn.getAccountID());
            if (it != accounts.end()) {
                it->second->loadTransaction(txn);
            }
        }
        return true;
    } catch (const StorageException&) {
        throw;
    } catch (const std::exception& e) {
        throw StorageException(std::string("loadTransactions failed: ") + e.what());
    }
}

// ── Convenience ───────────────────────────────────────────────────────────────

bool StorageManager::saveAll(const Bank& bank) {
    return saveUsers(bank.getUsers()) &&
           saveAccounts(bank.getAccounts()) &&
           saveTransactions(bank.getAccounts());
}

bool StorageManager::loadAll(Bank& bank) {
    return loadUsers(bank.m_users, bank.m_usernameIndex) &&
           loadAccounts(bank.m_accounts) &&
           loadTransactions(bank.m_accounts);
}

} // namespace novabanc
