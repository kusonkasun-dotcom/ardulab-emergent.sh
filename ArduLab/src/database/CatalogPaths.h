#pragma once

// CatalogPaths — resolve the per-user catalog location (Plan §4.6, ADR §2.3).
//
// Windows target: %LOCALAPPDATA%\ArduLab\catalog\ardulab_catalog.sqlite3
// This is expressed portably through QStandardPaths::AppLocalDataLocation so
// the same code resolves a per-user, admin-free, working-directory-independent
// path on every platform (Linux/CI included).

#include "core/Result.h"

#include <QString>

namespace ardulab::database {

class CatalogPaths
{
public:
    /// The catalog directory: <AppLocalData>/catalog (created if absent).
    [[nodiscard]] static core::Result<QString> catalogDir();

    /// The primary catalog database file, creating parent directories.
    [[nodiscard]] static core::Result<QString> defaultDatabaseFile();

    /// The backups directory: <AppLocalData>/catalog/backups (created if absent).
    [[nodiscard]] static core::Result<QString> backupsDir();

    static constexpr const char* kDatabaseFileName = "ardulab_catalog.sqlite3";
};

} // namespace ardulab::database
