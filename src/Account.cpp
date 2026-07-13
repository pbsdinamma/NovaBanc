#include "Account.h"
#include "Exceptions.h"
#include "IDGenerator.h"
#include "Utils.h"

#include <cmath>
#include <iomanip>
#include <sstream>

namespace novabanc {

// Constructor

Account::Account(const std::string &accountID, const std::string &userID,
                 double balance, AccountStatus status,
                 const std::string &openedDate, double dailyWithdrawnToday,
                 const std::string &lastWithdrawDate)
    : m_accountID(accountID), m_userID(userID), m_balance(balance),
      m_status(status), m_openedDate(openedDate),
      m_dailyWithdrawnToday(dailyWithdrawnToday),
      m_lastWithdrawDate(lastWithdrawDate) {}

// Status helpers

std::string Account::getStatusString() const {
  switch (m_status) {
  case AccountStatus::ACTIVE:
    return "ACTIVE";
  case AccountStatus::FROZEN:
    return "FROZEN";
  case AccountStatus::CLOSED:
    return "CLOSED";
  }
  return "UNKNOWN";
}

// Daily limit

void Account::refreshDailyLimit() {
  std::string today = utils::getCurrentDate();
  if (m_lastWithdrawDate != today) {
    m_dailyWithdrawnToday = 0.0;
    m_lastWithdrawDate = today;
  }
}

// Core operations

bool Account::deposit(double amount, const std::string &description) {
  if (isFrozen()) {
    throw AccountFrozenException("Account " + m_accountID + " is frozen.");
  }
  if (isClosed()) {
    throw InvalidInputException("Cannot deposit into a closed account.");
  }
  if (amount <= 0.0 || std::isnan(amount) || std::isinf(amount)) {
    throw InvalidInputException("Deposit amount must be a positive number.");
  }

  m_balance += amount;

  std::string ts = utils::getCurrentTimestamp();
  std::string id = IDGenerator::generateTransactionID();
  std::string desc = description.empty() ? "Deposit" : description;

  m_transactions.emplace_back(id, m_accountID, TransactionType::DEPOSIT, amount,
                              m_balance, ts, desc);
  return true;
}

bool Account::withdraw(double amount, const std::string &description) {
  if (amount <= 0.0 || std::isnan(amount) || std::isinf(amount)) {
    throw InvalidInputException("Withdrawal amount must be a positive number.");
  }
  if (isFrozen()) {
    throw AccountFrozenException("Account " + m_accountID + " is frozen.");
  }
  if (isClosed()) {
    throw InvalidInputException("Cannot withdraw from a closed account.");
  }

  // Refresh daily counters; then let subclass decide.
  refreshDailyLimit();

  if (!canWithdraw(amount)) {
    // canWithdraw returning false when the account is active means either
    // minimum balance or daily limit is violated. Determine which for a
    // useful error message.
    if (m_dailyWithdrawnToday + amount > DAILY_LIMIT) {
      throw InsufficientFundsException(
          "Daily withdrawal limit would be exceeded.");
    }
    throw InsufficientFundsException(
        "Withdrawal would violate minimum balance or account rules.");
  }

  m_balance -= amount;
  m_dailyWithdrawnToday += amount;

  std::string ts = utils::getCurrentTimestamp();
  std::string id = IDGenerator::generateTransactionID();
  std::string desc = description.empty() ? "Withdrawal" : description;

  m_transactions.emplace_back(id, m_accountID, TransactionType::WITHDRAWAL,
                              amount, m_balance, ts, desc);
  return true;
}

void Account::freeze() {
  if (!isClosed())
    m_status = AccountStatus::FROZEN;
}

void Account::unfreeze() {
  if (isFrozen())
    m_status = AccountStatus::ACTIVE;
}

bool Account::close() {
  if (isClosed())
    return false;
  if (std::abs(m_balance) > 0.01)
    return false; // balance must be 0
  m_status = AccountStatus::CLOSED;
  return true;
}

} // namespace novabanc
