#pragma once

// CatalogDatabase — owns the SQLite connection lifecycle (Plan §4.6, ADR §2.4).
//
// One CatalogDatabase == one uniquely-named Qt SQL connection. It enables
// foreign-key enforcement and the approved pragmas on open, translates raw
// QSqlError into core::Error, and hands a QSqlDatabase handle only to the
// database-layer adapters (never to the UI).
//
// Use ":memory:" as the file path for an ephemeral in-process catalog (tests).

#include "core/Result.h"

#include <QSqlDatabase>
#include <QString>

class QSqlError;

namespace ardulab::database {

class CatalogDatabase
{
public:
    explicit CatalogDatabase(QString filePath);
    ~CatalogDatabase();

    CatalogDatabase(const CatalogDatabase&) = delete;
    CatalogDatabase& operator=(const CatalogDatabase&) = delete;

    /// Open the connection and apply foreign_keys / busy_timeout / journal pragmas.
    [[nodiscard]] core::Status open();
    void close();
    [[nodiscard]] bool isOpen() const noexcept { return m_open; }

    /// The owning connection. Precondition: isOpen().
    [[nodiscard]] QSqlDatabase database() const;

    [[nodiscard]] const QString& filePath() const noexcept { return m_filePath; }
    [[nodiscard]] const QString& connectionName() const noexcept { return m_connectionName; }

    /// Translate a Qt SQL error into a domain error without leaking Qt types.
    [[nodiscard]] static core::Error translate(const QString& what, const QSqlError& error);

private:
    QString m_filePath;
    QString m_connectionName;
    bool m_open = false;
};

} // namespace ardulab::database
