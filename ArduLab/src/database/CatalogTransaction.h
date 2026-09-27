#pragma once

// CatalogTransaction — RAII commit/rollback ownership (Plan §4.6, ADR §2.5).
//
// begin() starts a transaction on the given connection. If the guard is
// destroyed without a successful commit(), the transaction is rolled back, so
// a failed multi-statement operation never leaves a partial write behind.

#include "core/Result.h"

#include <QSqlDatabase>
#include <QSqlError>

namespace ardulab::database {

class CatalogTransaction
{
public:
    explicit CatalogTransaction(QSqlDatabase db)
        : m_db(std::move(db))
    {
    }

    ~CatalogTransaction()
    {
        if (m_active && !m_committed) {
            m_db.rollback();
        }
    }

    CatalogTransaction(const CatalogTransaction&) = delete;
    CatalogTransaction& operator=(const CatalogTransaction&) = delete;

    [[nodiscard]] core::Status begin()
    {
        if (m_active) {
            return core::Status::success();
        }
        if (!m_db.transaction()) {
            return core::Error(core::ErrorCode::IoFailure, QStringLiteral("could not begin transaction"),
                               m_db.lastError().text());
        }
        m_active = true;
        return core::Status::success();
    }

    [[nodiscard]] core::Status commit()
    {
        if (!m_active) {
            return core::Error(core::ErrorCode::InvalidState, QStringLiteral("commit without an active transaction"));
        }
        if (!m_db.commit()) {
            return core::Error(core::ErrorCode::IoFailure, QStringLiteral("could not commit transaction"),
                               m_db.lastError().text());
        }
        m_committed = true;
        m_active = false;
        return core::Status::success();
    }

    void rollback()
    {
        if (m_active && !m_committed) {
            m_db.rollback();
            m_active = false;
        }
    }

    [[nodiscard]] bool isActive() const noexcept { return m_active; }

private:
    QSqlDatabase m_db;
    bool m_active = false;
    bool m_committed = false;
};

} // namespace ardulab::database
