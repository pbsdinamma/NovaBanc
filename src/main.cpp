// ─────────────────────────────────────────────────────────────────────────────
// NovaBanc — Terminal Banking Application
// Entry point and full menu system.
//
// Architecture:
//   main() initialises services, then runs the top-level menu loop.
//   Each sub-menu is a free function that runs its own loop.
//   All exceptions from business logic are caught at the menu level.
//   Nothing propagates past a menu function to main().
// ─────────────────────────────────────────────────────────────────────────────

#include <iostream>
#include <iomanip>
#include <memory>
#include <string>
#include <vector>

#include "AuthService.h"
#include "Bank.h"
#include "CSVExporter.h"
#include "CurrentAccount.h"
#include "SavingsAccount.h"
#include "Exceptions.h"
#include "IDGenerator.h"
#include "InputValidator.h"
#include "Logger.h"
#include "StorageManager.h"
#include "Transaction.h"
#include "User.h"
#include "Utils.h"

#ifdef _WIN32
#include <windows.h>
#endif

using namespace novabanc;
using namespace novabanc::utils;

// ─────────────────────────────────────────────────────────────────────────────
// Forward declarations
// ─────────────────────────────────────────────────────────────────────────────

void runMainMenu(Bank& bank, AuthService& auth, Logger& logger);
void runUserMenu(Bank& bank, AuthService& auth, Logger& logger);
void runAdminMenu(Bank& bank, AuthService& auth, Logger& logger);

// User sub-menus
void handleRegister(Bank& bank, Logger& logger);
void handleMyAccounts(Bank& bank, const std::string& userID);
void handleDeposit(Bank& bank, const std::string& userID, Logger& logger);
void handleWithdraw(Bank& bank, const std::string& userID, Logger& logger);
void handleTransfer(Bank& bank, const std::string& userID, Logger& logger);
void handleTransactionHistory(Bank& bank, const std::string& userID);
void handleProfileSettings(Bank& bank, const std::string& userID, AuthService& auth, Logger& logger);

// Admin sub-menus
void handleAdminUsers(Bank& bank, Logger& logger);
void handleAdminAccounts(Bank& bank, Logger& logger);
void handleAdminReports(Bank& bank);
void showBankStats(Bank& bank);

// ─────────────────────────────────────────────────────────────────────────────
// Helper: pick an account from the current user's accounts
// ─────────────────────────────────────────────────────────────────────────────

static std::string pickAccount(Bank& bank, const std::string& userID,
                                               bool activeOnly = true) {
    auto accounts = bank.getAccountsByUserID(userID);
    if (accounts.empty()) {
        printWarning("You have no accounts. Open one first.");
        return "";
    }

    std::vector<std::vector<std::string>> rows;
    int idx = 1;
    std::vector<std::string> validIDs;
    for (Account* acc : accounts) {
        if (activeOnly && !acc->isActive()) continue;
        rows.push_back({std::to_string(idx),
                        acc->getAccountID(),
                        acc->getAccountType(),
                        formatAmount(acc->getBalance()),
                        acc->getStatusString()});
        validIDs.push_back(acc->getAccountID());
        idx++;
    }
    if (validIDs.empty()) {
        printWarning("No active accounts available.");
        return "";
    }

    printTable({"#", "Account ID", "Type", "Balance", "Status"}, rows);
    int choice = promptInt("Select account number (0 to cancel): ");
    if (choice == 0) return "";
    if (choice < 1 || choice > static_cast<int>(validIDs.size())) {
        printError("Invalid selection.");
        return "";
    }
    return validIDs[static_cast<size_t>(choice - 1)];
}

// ─────────────────────────────────────────────────────────────────────────────
// Register a new user
// ─────────────────────────────────────────────────────────────────────────────

void handleRegister(Bank& bank, Logger& logger) {
    printHeader("Register New Account");

    std::string username, password, confirm;
    std::string firstName, lastName, email, phone, address;
    int age = 0;

    // Username
    while (true) {
        username = promptString("Username: ");
        if (username.empty()) { printError("Username cannot be empty."); continue; }
        if (bank.findUserByUsername(username)) {
            printError("Username already taken. Please choose another.");
            continue;
        }
        break;
    }

    // Password
    while (true) {
        std::cout << "  Password: ";
        password = getHiddenInput();
        if (!InputValidator::isValidPassword(password)) {
            printError("Password must be at least 8 characters and contain a digit.");
            continue;
        }
        std::cout << "  Confirm Password: ";
        confirm = getHiddenInput();
        if (password != confirm) { printError("Passwords do not match."); continue; }
        break;
    }

    // First name
    while (true) {
        firstName = promptString("First Name: ");
        if (!InputValidator::isValidName(firstName))
            { printError("Name must be 2–50 letters only."); continue; }
        break;
    }

    // Last name
    while (true) {
        lastName = promptString("Last Name: ");
        if (!InputValidator::isValidName(lastName))
            { printError("Name must be 2–50 letters only."); continue; }
        break;
    }

    // Email
    while (true) {
        email = promptString("Email: ");
        if (!InputValidator::isValidEmail(email))
            { printError("Invalid email address."); continue; }
        break;
    }

    // Phone
    while (true) {
        phone = promptString("Phone (10 digits): ");
        if (!InputValidator::isValidPhone(phone))
            { printError("Phone must be exactly 10 digits."); continue; }
        break;
    }

    // Age
    while (true) {
        age = promptInt("Age: ");
        if (!InputValidator::isValidAge(age))
            { printError("Age must be between 18 and 100."); continue; }
        break;
    }

    // Address
    address = promptString("Address: ");
    if (address.empty()) address = "Not provided";

    try {
        User& user = bank.createUser(username, password, firstName, lastName,
                                     email, phone, age, address);
        bank.saveAll();
        logger.info("New user registered: " + user.getUserID() + " (" + username + ")");
        printSuccess("Account created! Welcome to NovaBanc, " + firstName + ".");
        std::cout << "  Your User ID: " << user.getUserID() << "\n\n";
    } catch (const BankException& e) {
        printError(e.what());
        logger.error(std::string("Registration failed: ") + e.what());
    }

    promptString("Press Enter to continue...");
}

// ─────────────────────────────────────────────────────────────────────────────
// User menu — My Accounts
// ─────────────────────────────────────────────────────────────────────────────

void handleMyAccounts(Bank& bank, const std::string& userID) {
    while (true) {
        printHeader("My Accounts");
        auto accounts = bank.getAccountsByUserID(userID);

        if (accounts.empty()) {
            printWarning("You have no accounts yet.");
        } else {
            std::vector<std::vector<std::string>> rows;
            for (Account* acc : accounts) {
                rows.push_back({acc->getAccountID(),
                                acc->getAccountType(),
                                formatAmount(acc->getBalance()),
                                acc->getStatusString(),
                                acc->getOpenedDate().substr(0, 10)});
            }
            printTable({"Account ID", "Type", "Balance", "Status", "Opened"}, rows);
        }

        std::cout << "  [1] Open Savings Account\n"
                  << "  [2] Open Current Account\n"
                  << "  [3] Close an Account\n"
                  << "  [0] Back\n\n";

        int choice = promptInt("Enter choice: ");
        if (choice == 0) return;

        if (choice == 1) {
            // Open savings account.
            printHeader("Open Savings Account");
            std::cout << "  Minimum initial deposit: " << formatAmount(SavingsAccount::MIN_BALANCE) << "\n\n";
            double amount = promptDouble("Initial deposit amount: Rs.");
            if (!InputValidator::isValidAmount(amount)) {
                printError("Invalid amount."); promptString("Press Enter..."); continue;
            }
            try {
                auto acc = bank.createSavingsAccount(userID, amount);
                printSuccess("Savings account opened: " + acc->getAccountID());
            } catch (const BankException& e) {
                printError(e.what());
            }
        } else if (choice == 2) {
            // Open current account.
            printHeader("Open Current Account");
            std::cout << "  Minimum initial deposit: " << formatAmount(CurrentAccount::MIN_BALANCE) << "\n\n";
            double amount = promptDouble("Initial deposit amount: Rs.");
            if (!InputValidator::isValidAmount(amount)) {
                printError("Invalid amount."); promptString("Press Enter..."); continue;
            }
            try {
                auto acc = bank.createCurrentAccount(userID, amount);
                printSuccess("Current account opened: " + acc->getAccountID());
            } catch (const BankException& e) {
                printError(e.what());
            }
        } else if (choice == 3) {
            // Close an account.
            auto allAccounts = bank.getAccountsByUserID(userID);
            if (allAccounts.empty()) { printWarning("No accounts to close."); }
            else {
                std::string accIDOpt = pickAccount(bank, userID, false);
                if (!accIDOpt.empty()) {
                    Account* acc = bank.findAccountByID(accIDOpt);
                    if (acc && acc->getBalance() != 0.0) {
                        printError("Account balance must be Rs.0.00 before closing. Please withdraw funds first.");
                    } else if (confirmAction("Close account " + accIDOpt + "? This cannot be undone.")) {
                        try {
                            bool ok = bank.closeAccount(accIDOpt);
                            if (ok) printSuccess("Account closed.");
                            else    printError("Could not close account (non-zero balance).");
                        } catch (const BankException& e) {
                            printError(e.what());
                        }
                    }
                }
            }
        }

        promptString("Press Enter to continue...");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Deposit
// ─────────────────────────────────────────────────────────────────────────────

void handleDeposit(Bank& bank, const std::string& userID, Logger& logger) {
    printHeader("Deposit");
    std::string accIDOpt = pickAccount(bank, userID);
    if (accIDOpt.empty()) { promptString("Press Enter..."); return; }

    double amount = promptDouble("Deposit amount: Rs.");
    if (!InputValidator::isValidAmount(amount)) {
        printError("Invalid amount."); promptString("Press Enter..."); return;
    }
    std::string desc = promptString("Description (optional): ");

    try {
        bank.deposit(accIDOpt, amount, desc.empty() ? "Deposit" : desc);
        Account* acc = bank.findAccountByID(accIDOpt);
        printSuccess("Deposited " + formatAmount(amount) + ". New balance: " +
                     (acc ? formatAmount(acc->getBalance()) : "—"));
        logger.info("Deposit " + formatAmount(amount) + " → " + accIDOpt);
    } catch (const BankException& e) {
        printError(e.what());
        logger.error(std::string("Deposit failed: ") + e.what());
    }
    promptString("Press Enter to continue...");
}

// ─────────────────────────────────────────────────────────────────────────────
// Withdraw
// ─────────────────────────────────────────────────────────────────────────────

void handleWithdraw(Bank& bank, const std::string& userID, Logger& logger) {
    printHeader("Withdraw");
    std::string accIDOpt = pickAccount(bank, userID);
    if (accIDOpt.empty()) { promptString("Press Enter..."); return; }

    double amount = promptDouble("Withdrawal amount: Rs.");
    if (!InputValidator::isValidAmount(amount)) {
        printError("Invalid amount."); promptString("Press Enter..."); return;
    }
    std::string desc = promptString("Description (optional): ");

    try {
        bank.withdraw(accIDOpt, amount, desc.empty() ? "Withdrawal" : desc);
        Account* acc = bank.findAccountByID(accIDOpt);
        printSuccess("Withdrew " + formatAmount(amount) + ". New balance: " +
                     (acc ? formatAmount(acc->getBalance()) : "—"));
        logger.info("Withdrawal " + formatAmount(amount) + " ← " + accIDOpt);
    } catch (const InsufficientFundsException& e) {
        printError(std::string("Insufficient funds — ") + e.what());
        logger.warn(std::string("Withdrawal failed: ") + e.what());
    } catch (const AccountFrozenException& e) {
        printError("This account is frozen. Please contact the bank administrator.");
        logger.warn(std::string("Withdrawal blocked (frozen): ") + accIDOpt);
    } catch (const BankException& e) {
        printError(e.what());
        logger.error(std::string("Withdrawal failed: ") + e.what());
    }
    promptString("Press Enter to continue...");
}

// ─────────────────────────────────────────────────────────────────────────────
// Transfer
// ─────────────────────────────────────────────────────────────────────────────

void handleTransfer(Bank& bank, const std::string& userID, Logger& logger) {
    printHeader("Transfer Funds");
    std::cout << "  [1] Transfer between my accounts\n"
              << "  [2] Transfer to another user's account\n"
              << "  [0] Back\n\n";

    int choice = promptInt("Enter choice: ");
    if (choice == 0) return;

    // Source account.
    std::string fromOpt = pickAccount(bank, userID);
    if (fromOpt.empty()) { promptString("Press Enter..."); return; }

    std::string toID;
    if (choice == 1) {
        // Destination must also be user's own account.
        std::cout << "\n  Select destination account:\n";
        auto accounts = bank.getAccountsByUserID(userID);
        std::vector<std::vector<std::string>> rows;
        int idx = 1;
        std::vector<std::string> validIDs;
        for (Account* acc : accounts) {
            if (acc->getAccountID() == fromOpt) continue;
            if (!acc->isActive()) continue;
            rows.push_back({std::to_string(idx), acc->getAccountID(),
                            acc->getAccountType(), formatAmount(acc->getBalance())});
            validIDs.push_back(acc->getAccountID());
            idx++;
        }
        if (validIDs.empty()) {
            printWarning("No other active accounts to transfer to.");
            promptString("Press Enter..."); return;
        }
        printTable({"#", "Account ID", "Type", "Balance"}, rows);
        int sel = promptInt("Select destination (0 to cancel): ");
        if (sel == 0) return;
        if (sel < 1 || sel > static_cast<int>(validIDs.size())) {
            printError("Invalid selection."); promptString("Press Enter..."); return;
        }
        toID = validIDs[static_cast<size_t>(sel - 1)];
    } else {
        toID = promptString("Enter destination Account ID: ");
        Account* destAcc = bank.findAccountByID(toID);
        if (!destAcc) {
            printError("Account not found: " + toID);
            promptString("Press Enter..."); return;
        }
        std::cout << "  Destination: " << toID << " (" << destAcc->getAccountType()
                  << ") — " << formatAmount(destAcc->getBalance()) << "\n\n";
    }

    double amount = promptDouble("Transfer amount: Rs.");
    if (!InputValidator::isValidAmount(amount)) {
        printError("Invalid amount."); promptString("Press Enter..."); return;
    }

    if (!confirmAction("Transfer " + formatAmount(amount) + " from " + fromOpt + " to " + toID + "?")) {
        promptString("Press Enter..."); return;
    }

    try {
        bank.transfer(fromOpt, toID, amount);
        printSuccess("Transfer complete — " + formatAmount(amount) + " sent to " + toID);
        logger.info("Transfer " + formatAmount(amount) + " from " + fromOpt + " to " + toID);
    } catch (const BankException& e) {
        printError(e.what());
        logger.error(std::string("Transfer failed: ") + e.what());
    }
    promptString("Press Enter to continue...");
}

// ─────────────────────────────────────────────────────────────────────────────
// Transaction History
// ─────────────────────────────────────────────────────────────────────────────

void handleTransactionHistory(Bank& bank, const std::string& userID) {
    printHeader("Transaction History");
    std::string accIDOpt = pickAccount(bank, userID, false);
    if (accIDOpt.empty()) { promptString("Press Enter..."); return; }

    std::cout << "  [1] All transactions\n"
              << "  [2] Deposits only\n"
              << "  [3] Withdrawals only\n"
              << "  [4] Transfers only\n\n";
    int filter = promptInt("Filter: ");

    try {
        auto txns = bank.getTransactionHistory(accIDOpt);
        std::vector<Transaction> filtered;
        for (const auto& t : txns) {
            if (filter == 1) {
                filtered.push_back(t);
            } else if (filter == 2 && t.getType() == TransactionType::DEPOSIT) {
                filtered.push_back(t);
            } else if (filter == 3 && t.getType() == TransactionType::WITHDRAWAL) {
                filtered.push_back(t);
            } else if (filter == 4 && (t.getType() == TransactionType::TRANSFER_DEBIT ||
                                        t.getType() == TransactionType::TRANSFER_CREDIT)) {
                filtered.push_back(t);
            }
        }

        if (filtered.empty()) {
            printWarning("No transactions found.");
        } else {
            std::vector<std::vector<std::string>> rows;
            for (const auto& t : filtered) {
                rows.push_back({t.getTimestamp().substr(0, 10),
                                t.getTypeString(),
                                formatAmount(t.getAmount()),
                                formatAmount(t.getBalanceAfter()),
                                t.getDescription()});
            }
            printTable({"Date", "Type", "Amount", "Balance After", "Description"}, rows);
            std::cout << "  Total: " << filtered.size() << " transaction(s)\n\n";
        }
    } catch (const BankException& e) {
        printError(e.what());
    }
    promptString("Press Enter to continue...");
}

// ─────────────────────────────────────────────────────────────────────────────
// Profile Settings
// ─────────────────────────────────────────────────────────────────────────────

void handleProfileSettings(Bank& bank, const std::string& userID,
                           AuthService& auth, Logger& logger) {
    while (true) {
        printHeader("Profile Settings");
        User* user = bank.findUserByID(userID);
        if (!user) { printError("User not found."); return; }

        std::cout << "  Name:    " << user->getFullName()  << "\n"
                  << "  Email:   " << user->getEmail()     << "\n"
                  << "  Phone:   " << user->getPhone()     << "\n"
                  << "  Age:     " << user->getAge()       << "\n"
                  << "  Address: " << user->getAddress()   << "\n"
                  << "  User ID: " << user->getUserID()    << "\n\n";

        printDivider();
        std::cout << "  [1] Update Email\n"
                  << "  [2] Update Phone\n"
                  << "  [3] Change Password\n"
                  << "  [0] Back\n\n";

        int choice = promptInt("Enter choice: ");
        if (choice == 0) return;

        if (choice == 1) {
            std::string email = promptString("New email: ");
            try {
                bank.updateUserEmail(userID, email);
                bank.saveAll();
                printSuccess("Email updated.");
            } catch (const BankException& e) { printError(e.what()); }
        } else if (choice == 2) {
            std::string phone = promptString("New phone (10 digits): ");
            try {
                bank.updateUserPhone(userID, phone);
                bank.saveAll();
                printSuccess("Phone updated.");
            } catch (const BankException& e) { printError(e.what()); }
        } else if (choice == 3) {
            std::cout << "  Current Password: ";
            std::string current = getHiddenInput();
            if (!user->verifyPassword(current)) {
                printError("Incorrect current password.");
            } else {
                std::cout << "  New Password: ";
                std::string pw1 = getHiddenInput();
                std::cout << "  Confirm New Password: ";
                std::string pw2 = getHiddenInput();
                if (pw1 != pw2) {
                    printError("Passwords do not match.");
                } else if (!InputValidator::isValidPassword(pw1)) {
                    printError("Password must be at least 8 characters and contain a digit.");
                } else {
                    user->setPassword(pw1);
                    bank.saveAll();
                    logger.info("Password changed for user " + userID);
                    printSuccess("Password changed successfully.");
                }
            }
        }

        promptString("Press Enter to continue...");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// User Menu
// ─────────────────────────────────────────────────────────────────────────────

void runUserMenu(Bank& bank, AuthService& auth, Logger& logger) {
    while (true) {
        User* user = bank.findUserByID(auth.getCurrentUserID());
        std::string name = user ? user->getFullName() : "User";

        printHeader("NovaBanc  —  Welcome, " + name);

        std::cout << "  [1] My Accounts\n"
                  << "  [2] Deposit\n"
                  << "  [3] Withdraw\n"
                  << "  [4] Transfer\n"
                  << "  [5] Transaction History\n"
                  << "  [6] Profile Settings\n"
                  << "  [0] Logout\n\n";

        int choice = promptInt("Enter choice: ");

        if      (choice == 0) { auth.logout(); logger.info("User logged out: " + auth.getCurrentUserID()); break; }
        else if (choice == 1) handleMyAccounts(bank, auth.getCurrentUserID());
        else if (choice == 2) handleDeposit(bank, auth.getCurrentUserID(), logger);
        else if (choice == 3) handleWithdraw(bank, auth.getCurrentUserID(), logger);
        else if (choice == 4) handleTransfer(bank, auth.getCurrentUserID(), logger);
        else if (choice == 5) handleTransactionHistory(bank, auth.getCurrentUserID());
        else if (choice == 6) handleProfileSettings(bank, auth.getCurrentUserID(), auth, logger);
        else printError("Invalid choice. Please enter 0–6.");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Admin — Users sub-menu
// ─────────────────────────────────────────────────────────────────────────────

void handleAdminUsers(Bank& bank, Logger& logger) {
    while (true) {
        printHeader("Admin — User Management");
        std::cout << "  [1] List all users\n"
                  << "  [2] Search users\n"
                  << "  [3] View user details\n"
                  << "  [4] Lock user\n"
                  << "  [5] Unlock user\n"
                  << "  [6] Delete user\n"
                  << "  [0] Back\n\n";

        int choice = promptInt("Enter choice: ");
        if (choice == 0) return;

        if (choice == 1) {
            auto users = bank.getAllUsers();
            if (users.empty()) { printWarning("No users registered."); }
            else {
                std::vector<std::vector<std::string>> rows;
                for (User* u : users) {
                    rows.push_back({u->getUserID(), u->getUsername(), u->getFullName(),
                                    u->getEmail(), u->isLocked() ? "LOCKED" : "ACTIVE",
                                    std::to_string(u->getAccountIDs().size())});
                }
                printTable({"User ID", "Username", "Name", "Email", "Status", "Accounts"}, rows);
            }
        } else if (choice == 2) {
            std::string query = promptString("Search (name or email): ");
            auto results = bank.searchUsers(query);
            if (results.empty()) { printWarning("No matches found."); }
            else {
                std::vector<std::vector<std::string>> rows;
                for (User* u : results) {
                    rows.push_back({u->getUserID(), u->getUsername(), u->getFullName(), u->getEmail()});
                }
                printTable({"User ID", "Username", "Name", "Email"}, rows);
            }
        } else if (choice == 3) {
            std::string uid = promptString("Enter User ID: ");
            User* u = bank.findUserByID(uid);
            if (!u) { printError("User not found."); }
            else {
                printDivider();
                std::cout << "  User ID:    " << u->getUserID()    << "\n"
                          << "  Username:   " << u->getUsername()   << "\n"
                          << "  Full Name:  " << u->getFullName()   << "\n"
                          << "  Email:      " << u->getEmail()      << "\n"
                          << "  Phone:      " << u->getPhone()      << "\n"
                          << "  Age:        " << u->getAge()        << "\n"
                          << "  Address:    " << u->getAddress()    << "\n"
                          << "  Created:    " << u->getCreatedAt()  << "\n"
                          << "  Status:     " << (u->isLocked() ? "LOCKED" : "ACTIVE") << "\n"
                          << "  Accounts:   " << u->getAccountIDs().size() << "\n\n";
            }
        } else if (choice == 4) {
            std::string uid = promptString("Enter User ID to lock: ");
            User* u = bank.findUserByID(uid);
            if (!u) { printError("User not found."); }
            else if (confirmAction("Lock user " + u->getUsername() + "?")) {
                u->lock();
                bank.saveAll();
                logger.info("Admin locked user: " + uid);
                printSuccess("User locked.");
            }
        } else if (choice == 5) {
            std::string uid = promptString("Enter User ID to unlock: ");
            User* u = bank.findUserByID(uid);
            if (!u) { printError("User not found."); }
            else {
                u->unlock();
                bank.saveAll();
                logger.info("Admin unlocked user: " + uid);
                printSuccess("User unlocked. Failed attempts reset.");
            }
        } else if (choice == 6) {
            std::string uid = promptString("Enter User ID to delete: ");
            if (confirmAction("Delete user " + uid + "? This cannot be undone.")) {
                try {
                    bool ok = bank.deleteUser(uid);
                    if (ok) {
                        bank.saveAll();
                        logger.info("Admin deleted user: " + uid);
                        printSuccess("User deleted.");
                    } else {
                        printError("Cannot delete user — they have open accounts.");
                    }
                } catch (const BankException& e) {
                    printError(e.what());
                }
            }
        }

        promptString("Press Enter to continue...");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Admin — Accounts sub-menu
// ─────────────────────────────────────────────────────────────────────────────

void handleAdminAccounts(Bank& bank, Logger& logger) {
    while (true) {
        printHeader("Admin — Account Management");
        std::cout << "  [1] List all accounts\n"
                  << "  [2] View account details\n"
                  << "  [3] Freeze account\n"
                  << "  [4] Unfreeze account\n"
                  << "  [5] Apply monthly interest (all savings)\n"
                  << "  [0] Back\n\n";

        int choice = promptInt("Enter choice: ");
        if (choice == 0) return;

        if (choice == 1) {
            auto accounts = bank.getAllAccounts();
            if (accounts.empty()) { printWarning("No accounts."); }
            else {
                std::vector<std::vector<std::string>> rows;
                for (Account* acc : accounts) {
                    rows.push_back({acc->getAccountID(), acc->getUserID(),
                                    acc->getAccountType(), formatAmount(acc->getBalance()),
                                    acc->getStatusString()});
                }
                printTable({"Account ID", "User ID", "Type", "Balance", "Status"}, rows);
            }
        } else if (choice == 2) {
            std::string aid = promptString("Enter Account ID: ");
            Account* acc = bank.findAccountByID(aid);
            if (!acc) { printError("Account not found."); }
            else {
                printDivider();
                std::cout << "  Account ID:   " << acc->getAccountID()    << "\n"
                          << "  User ID:      " << acc->getUserID()       << "\n"
                          << "  Type:         " << acc->getAccountType()  << "\n"
                          << "  Balance:      " << formatAmount(acc->getBalance()) << "\n"
                          << "  Status:       " << acc->getStatusString() << "\n"
                          << "  Opened:       " << acc->getOpenedDate()   << "\n"
                          << "  Transactions: " << acc->getTransactions().size() << "\n\n";
            }
        } else if (choice == 3) {
            std::string aid = promptString("Enter Account ID to freeze: ");
            if (confirmAction("Freeze account " + aid + "?")) {
                try {
                    bank.freezeAccount(aid);
                    bank.saveAll();
                    printSuccess("Account frozen.");
                } catch (const BankException& e) { printError(e.what()); }
            }
        } else if (choice == 4) {
            std::string aid = promptString("Enter Account ID to unfreeze: ");
            try {
                bank.unfreezeAccount(aid);
                bank.saveAll();
                printSuccess("Account unfrozen.");
            } catch (const BankException& e) { printError(e.what()); }
        } else if (choice == 5) {
            if (confirmAction("Apply monthly interest to all active savings accounts?")) {
                bank.applyMonthlyInterestAll();
                logger.info("Admin applied monthly interest to all savings accounts.");
                printSuccess("Monthly interest applied.");
            }
        }

        promptString("Press Enter to continue...");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Admin — Reports
// ─────────────────────────────────────────────────────────────────────────────

void handleAdminReports(Bank& bank) {
    while (true) {
        printHeader("Admin — Reports");
        std::cout << "  [1] Export user report (CSV)\n"
                  << "  [2] Export all transactions (CSV)\n"
                  << "  [3] Export account summary (CSV)\n"
                  << "  [4] Export account statement (CSV)\n"
                  << "  [0] Back\n\n";

        int choice = promptInt("Enter choice: ");
        if (choice == 0) return;

        if (choice == 1) {
            std::string path = "reports/users_" + getCurrentDate() + ".csv";
            auto users = bank.getAllUsers();
            bool ok = CSVExporter::exportUsers(users, path);
            ok ? printSuccess("Exported to " + path) : printError("Export failed.");
        } else if (choice == 2) {
            std::string path = "reports/transactions_" + getCurrentDate() + ".csv";
            std::vector<Transaction> allTxns;
            for (Account* acc : bank.getAllAccounts()) {
                for (const auto& t : acc->getTransactions()) allTxns.push_back(t);
            }
            bool ok = CSVExporter::exportTransactions(allTxns, path);
            ok ? printSuccess("Exported to " + path) : printError("Export failed.");
        } else if (choice == 3) {
            std::string path = "reports/accounts_" + getCurrentDate() + ".csv";
            auto accounts = bank.getAllAccounts();
            bool ok = CSVExporter::exportAccountSummary(accounts, path);
            ok ? printSuccess("Exported to " + path) : printError("Export failed.");
        } else if (choice == 4) {
            std::string aid = promptString("Enter Account ID for statement: ");
            Account* acc = bank.findAccountByID(aid);
            if (!acc) { printError("Account not found."); }
            else {
                std::string path = "reports/statement_" + aid + "_" + getCurrentDate() + ".csv";
                bool ok = CSVExporter::exportUserStatement(*acc, path);
                ok ? printSuccess("Statement exported to " + path) : printError("Export failed.");
            }
        }

        promptString("Press Enter to continue...");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Bank Statistics
// ─────────────────────────────────────────────────────────────────────────────

void showBankStats(Bank& bank) {
    printHeader("Bank Statistics Dashboard");
    BankStats s = bank.getStats();

    std::cout << "  Users\n";
    printDivider();
    std::cout << "  Total Users:         " << s.totalUsers    << "\n"
              << "  Locked Users:        " << s.lockedUsers   << "\n\n";

    std::cout << "  Accounts\n";
    printDivider();
    std::cout << "  Total Accounts:      " << s.totalAccounts  << "\n"
              << "  Active:              " << s.activeAccounts  << "\n"
              << "  Frozen:              " << s.frozenAccounts  << "\n"
              << "  Savings:             " << s.savingsAccounts << "\n"
              << "  Current:             " << s.currentAccounts << "\n\n";

    std::cout << "  Financials\n";
    printDivider();
    std::cout << "  Total Balance:       " << formatAmount(s.totalBalance)     << "\n"
              << "  Total Transactions:  " << s.totalTransactions               << "\n\n";
    promptString("Press Enter to continue...");
}

// ─────────────────────────────────────────────────────────────────────────────
// Admin Menu
// ─────────────────────────────────────────────────────────────────────────────

void runAdminMenu(Bank& bank, AuthService& auth, Logger& logger) {
    while (true) {
        printHeader("NovaBanc  —  Admin Portal");
        std::cout << "  [1] User Management\n"
                  << "  [2] Account Management\n"
                  << "  [3] Reports\n"
                  << "  [4] Bank Statistics\n"
                  << "  [0] Logout\n\n";

        int choice = promptInt("Enter choice: ");

        if      (choice == 0) { auth.logout(); logger.info("Admin logged out."); break; }
        else if (choice == 1) handleAdminUsers(bank, logger);
        else if (choice == 2) handleAdminAccounts(bank, logger);
        else if (choice == 3) handleAdminReports(bank);
        else if (choice == 4) showBankStats(bank);
        else printError("Invalid choice. Please enter 0–4.");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Main Menu
// ─────────────────────────────────────────────────────────────────────────────

void runMainMenu(Bank& bank, AuthService& auth, Logger& logger) {
    while (true) {
        printHeader("NovaBanc  v1.0.0");
        std::cout << "  [1] Login as User\n"
                  << "  [2] Login as Admin\n"
                  << "  [3] Register\n"
                  << "  [0] Exit\n\n";

        int choice = promptInt("Enter choice: ");

        if (choice == 0) {
            printSuccess("Thank you for using NovaBanc. Goodbye!");
            return;
        } else if (choice == 1) {
            printHeader("User Login");
            std::string username = promptString("Username: ");
            std::cout << "  Password: ";
            std::string password = getHiddenInput();

            try {
                auth.loginUser(username, password, bank);
                logger.info("User logged in: " + username);
                runUserMenu(bank, auth, logger);
            } catch (const AuthenticationException& e) {
                printError(e.what());
                logger.warn(std::string("Failed login for: ") + username);
            }
        } else if (choice == 2) {
            printHeader("Admin Login");
            std::string username = promptString("Admin Username: ");
            std::cout << "  Admin Password: ";
            std::string password = getHiddenInput();

            try {
                auth.loginAdmin(username, password);
                logger.info("Admin logged in.");
                runAdminMenu(bank, auth, logger);
            } catch (const AuthenticationException& e) {
                printError(e.what());
                logger.warn("Failed admin login attempt.");
            }
        } else if (choice == 3) {
            handleRegister(bank, logger);
        } else {
            printError("Invalid choice. Please enter 0–3.");
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// main()
// ─────────────────────────────────────────────────────────────────────────────

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    // Initialise services.
    Logger         logger("logs/novabanc.log", LogLevel::INFO);
    StorageManager storage("data");
    Bank           bank("NovaBanc", storage, logger);
    AuthService    auth;

    IDGenerator::initialize("data/counters.json");

    // Load persisted data.
    try {
        bank.loadAll();
        logger.info("NovaBanc started. Data loaded successfully.");
    } catch (const StorageException& e) {
        // Corrupt data file — log and continue with empty state.
        logger.error(std::string("Data load warning: ") + e.what());
        std::cerr << "Warning: could not load data — starting fresh.\n";
    }

    // Run the main menu loop.
    try {
        runMainMenu(bank, auth, logger);
    } catch (const std::exception& e) {
        // Last-resort catch — should never be reached.
        std::cerr << "Fatal error: " << e.what() << "\n";
        logger.error(std::string("Fatal: ") + e.what());
        return 1;
    }

    // Save on exit.
    try {
        bank.saveAll();
        logger.info("NovaBanc shut down cleanly.");
    } catch (const StorageException& e) {
        logger.error(std::string("Save on exit failed: ") + e.what());
        std::cerr << "Warning: could not save data on exit.\n";
    }

    return 0;
}
