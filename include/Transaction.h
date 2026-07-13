#pragma once

#include <string>
#include <nlohmann/json.hpp>

// ─────────────────────────────────────────────────────────────────────────────
// Transaction — an immutable record of a single financial event.
//
// Once a Transaction is created, nothing about it changes. The history is
// therefore append-only and fully reliable. Every deposit, withdrawal, transfer
// debit/credit, and interest credit produces exactly one Transaction.
// ─────────────────────────────────────────────────────────────────────────────

namespace novabanc {

enum class TransactionType {
    DEPOSIT,
    WITHDRAWAL,
    TRANSFER_DEBIT,
    TRANSFER_CREDIT,
    INTEREST_CREDIT
};

class Transaction {
public:
    /// Construct a Transaction with all fields supplied.
    Transaction(const std::string& id,
                const std::string& accountID,
                TransactionType    type,
                double             amount,
                double             balanceAfter,
                const std::string& timestamp,
                const std::string& description);

    // ── Getters (all const) ──────────────────────────────────────────────────
    const std::string& getID()          const { return m_id; }
    const std::string& getAccountID()   const { return m_accountID; }
    TransactionType    getType()        const { return m_type; }
    double             getAmount()      const { return m_amount; }
    double             getBalanceAfter()const { return m_balanceAfter; }
    const std::string& getTimestamp()   const { return m_timestamp; }
    const std::string& getDescription() const { return m_description; }

    /// Human-readable type name ("Deposit", "Withdrawal", etc.).
    std::string getTypeString() const;

    // ── Serialization ────────────────────────────────────────────────────────
    nlohmann::json toJson() const;
    static Transaction fromJson(const nlohmann::json& j);

    /// Returns a CSV row matching the columns in csvHeader().
    std::string toCSVRow() const;
    static std::string csvHeader();

private:
    std::string     m_id;
    std::string     m_accountID;
    TransactionType m_type;
    double          m_amount;
    double          m_balanceAfter;
    std::string     m_timestamp;
    std::string     m_description;

    static std::string typeToString(TransactionType t);
    static TransactionType typeFromString(const std::string& s);
};

} // namespace novabanc
