#pragma once

// IComponentImporter — application-facing JSON import port (ADR §10.2 import_json).
//
// Kept in the Component Engine so the UI can trigger imports without knowing
// about SQLite or JSON parsing. The concrete implementation lives in the
// database/import layer and stores through the Component Manager, so the
// forbidden edge  ui ─X→ database/json-parsing  is never created.

#include "components/Component.h"
#include "components/ImportReport.h"
#include "core/Result.h"

#include <QByteArray>
#include <QString>

namespace ardulab::components {

class IComponentImporter
{
public:
    virtual ~IComponentImporter() = default;

    /// Import a canonical component JSON file. The returned report describes
    /// the outcome (Imported / Skipped / Rejected) with per-rule messages.
    /// A core::Error is returned only when the file cannot be read at all;
    /// schema/validation problems are reported inside the ImportReport.
    [[nodiscard]] virtual core::Result<ImportReport>
    importFromFile(const QString& filePath, CatalogScope scope) = 0;

    /// Import canonical component JSON from an in-memory buffer.
    [[nodiscard]] virtual core::Result<ImportReport>
    importFromJson(const QByteArray& bytes, CatalogScope scope, const QString& sourceReference) = 0;
};

} // namespace ardulab::components
