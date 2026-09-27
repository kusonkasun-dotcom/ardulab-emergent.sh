#include "database/CatalogDatabase.h"

#include <QAtomicInteger>
#include <QSqlError>
#include <QSqlQuery>

namespace ardulab::database {

namespace {

QString nextConnectionName()
{
    static QAtomicInteger<quint64> counter{0};
    return QStringLiteral("ardulab_catalog_%1").arg(counter.fetchAndAddOrdered(1));
}

// Best-effort pragma; only a hard SQL failure is reported. A pragma that
// returns an unexpected value (e.g. journal_mode on :memory:) is not fatal.
core::Status execPragma(QSqlDatabase& db, const QString& sql)
{
    QSqlQuery query(db);
    if (!query.exec(sql)) {
        return CatalogDatabase::translate(QStringLiteral("pragma failed: %1").arg(sql), query.lastError());
    }
    return core::Status::success();
}

} // namespace

CatalogDatabase::CatalogDatabase(QString filePath)
    : m_filePath(std::move(filePath))
    , m_connectionName(nextConnectionName())
{
}

CatalogDatabase::~CatalogDatabase()
{
    close();
}

core::Status CatalogDatabase::open()
{
    if (m_open) {
        return core::Status::success();
    }
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
        db.setDatabaseName(m_filePath);
        if (!db.open()) {
            const core::Error error = translate(QStringLiteral("could not open catalog database"), db.lastError());
            QSqlDatabase::removeDatabase(m_connectionName);
            return error;
        }

        // Foreign keys must be enforced on every connection (ADR §2.4).
        if (const auto fk = execPragma(db, QStringLiteral("PRAGMA foreign_keys = ON")); !fk) {
            db.close();
            QSqlDatabase::removeDatabase(m_connectionName);
            return fk;
        }
        (void)execPragma(db, QStringLiteral("PRAGMA busy_timeout = 5000"));
        (void)execPragma(db, QStringLiteral("PRAGMA journal_mode = WAL"));
        (void)execPragma(db, QStringLiteral("PRAGMA synchronous = NORMAL"));
    }
    m_open = true;
    return core::Status::success();
}

void CatalogDatabase::close()
{
    if (!m_open && !QSqlDatabase::contains(m_connectionName)) {
        return;
    }
    {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName, /*open=*/false);
        if (db.isValid() && db.isOpen()) {
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(m_connectionName);
    m_open = false;
}

QSqlDatabase CatalogDatabase::database() const
{
    return QSqlDatabase::database(m_connectionName, /*open=*/false);
}

core::Error CatalogDatabase::translate(const QString& what, const QSqlError& error)
{
    core::ErrorCode code = core::ErrorCode::IoFailure;
    // SQLite constraint violations surface as a distinct kind of failure.
    if (error.nativeErrorCode() == QLatin1String("19")) { // SQLITE_CONSTRAINT
        code = core::ErrorCode::AlreadyExists;
    }
    QString detail = error.text().trimmed();
    return core::Error(code, what, detail);
}

} // namespace ardulab::database
