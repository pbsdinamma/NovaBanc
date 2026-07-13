#include "Transaction.h"

#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace novabanc {

// ── Constructor ───────────────────────────────────────────────────────────────

Transaction::Transaction(const std::string& id,
                         const std::string& accountID,
                         TransactionType    type,
                         double             amount,
                         double             balanceAfter,
                         const std::string& timestamp,
                         const std::string& description)
    : m_id(id)
    , m_accountID(accountID)
    , m_type(type)
    , m_amount(amount)
    , m_balanceAfter(balanceAfter)
    , m_timestamp(timestamp)
    , m_description(description) {}

// ── Type string conversion ────────────────────────────────────────────────────

std::string Transaction::typeToString(TransactionType t) {
    switch (t) {
        case TransactionType::DEPOSIT:         return "DEPOSIT";
        case TransactionType::WITHDRAWAL:      return "WITHDRAWAL";
        case TransactionType::TRANSFER_DEBIT:  return "TRANSFER_DEBIT";
        case TransactionType::TRANSFER_CREDIT: return "TRANSFER_CREDIT";
        case TransactionType::INTEREST_CREDIT: return "INTEREST_CREDIT";
    }
    return "UNKNOWN";
}

TransactionType Transaction::typeFromString(const std::string& s) {
    if (s == "DEPOSIT")         return TransactionType::DEPOSIT;
    if (s == "WITHDRAWAL")      return TransactionType::WITHDRAWAL;
    if (s == "TRANSFER_DEBIT")  return TransactionType::TRANSFER_DEBIT;
    if (s == "TRANSFER_CREDIT") return TransactionType::TRANSFER_CREDIT;
    if (s == "INTEREST_CREDIT") return TransactionType::INTEREST_CREDIT;
    throw std::invalid_argument("Unknown TransactionType: " + s);
}

std::string Transaction::getTypeString() const {
    switch (m_type) {
        case TransactionType::DEPOSIT:         return "Deposit";
        case TransactionType::WITHDRAWAL:      return "Withdrawal";
        case TransactionType::TRANSFER_DEBIT:  return "Transfer (Debit)";
        case TransactionType::TRANSFER_CREDIT: return "Transfer (Credit)";
        case TransactionType::INTEREST_CREDIT: return "Interest Credit";
    }
    return "Unknown";
}

// ── JSON serialization ────────────────────────────────────────────────────────

nlohmann::json Transaction::toJson() const {
    return {
        {"transaction_id", m_id},
        {"account_id",     m_accountID},
        {"type",           typeToString(m_type)},
        {"amount",         m_amount},
        {"balance_after",  m_balanceAfter},
        {"timestamp",      m_timestamp},
        {"description",    m_description}
    };
}

Transaction Transaction::fromJson(const nlohmann::json& j) {
    return Transaction(
        j.at("transaction_id").get<std::string>(),
        j.at("account_id").get<std::string>(),
        typeFromString(j.at("type").get<std::string>()),
        j.at("amount").get<double>(),
        j.at("balance_after").get<double>(),
        j.at("timestamp").get<std::string>(),
        j.at("description").get<std::string>()
    );
}

// ── CSV serialization ─────────────────────────────────────────────────────────

// Wrap a field in double quotes if it contains a comma.
static std::string csvField(const std::string& s) {
    if (s.find(',') != std::string::npos) {
        return "\"" + s + "\"";
    }
    return s;
}

std::string Transaction::csvHeader() {
    return "transaction_id,account_id,type,amount,balance_after,timestamp,description";
}

std::string Transaction::toCSVRow() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << csvField(m_id)          << ","
        << csvField(m_accountID)   << ","
        << typeToString(m_type)    << ","
        << m_amount                << ","
        << m_balanceAfter          << ","
        << csvField(m_timestamp)   << ","
        << csvField(m_description);
    return oss.str();
}

} // namespace novabanc
