#include "database/CatalogPaths.h"

#include <QDir>
#include <QStandardPaths>

namespace ardulab::database {

namespace {

core::Result<QString> ensureDir(const QString& path)
{
    if (path.isEmpty()) {
        return core::Error(core::ErrorCode::IoFailure,
                           QStringLiteral("could not resolve a writable application data location"));
    }
    QDir dir(path);
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        return core::Error(core::ErrorCode::IoFailure,
                           QStringLiteral("could not create catalog directory"), path);
    }
    return path;
}

} // namespace

core::Result<QString> CatalogPaths::catalogDir()
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    return ensureDir(QDir(base).filePath(QStringLiteral("catalog")));
}

core::Result<QString> CatalogPaths::defaultDatabaseFile()
{
    const auto dir = catalogDir();
    if (!dir) {
        return dir.error();
    }
    return QDir(dir.value()).filePath(QString::fromLatin1(kDatabaseFileName));
}

core::Result<QString> CatalogPaths::backupsDir()
{
    const auto dir = catalogDir();
    if (!dir) {
        return dir.error();
    }
    return ensureDir(QDir(dir.value()).filePath(QStringLiteral("backups")));
}

} // namespace ardulab::database
