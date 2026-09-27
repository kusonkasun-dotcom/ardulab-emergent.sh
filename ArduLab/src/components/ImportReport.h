#pragma once

// ImportReport — outcome of a JSON component import (ADR §6.6, §8.3).
//
// A value type in the Component Engine so the UI and importer can share it
// without either depending on SQLite or JSON. Carries per-rule messages
// (VALxxx), the resolved identity, and the final outcome.

#include "components/Component.h"
#include "core/Identifiers.h"

#include <QString>

#include <vector>

namespace ardulab::components {

enum class ImportSeverity {
    Info,
    Warning,
    Error,
};

constexpr const char* importSeverityName(ImportSeverity s) noexcept
{
    switch (s) {
    case ImportSeverity::Info:    return "INFO";
    case ImportSeverity::Warning: return "WARNING";
    case ImportSeverity::Error:   return "ERROR";
    }
    return "INFO";
}

/// One validation/mapping message (ADR §8.3 validation report row).
struct ImportMessage final
{
    QString ruleId;                                   ///< "VAL005", "VAL016", or "" for generic.
    ImportSeverity severity = ImportSeverity::Info;
    QString message;                                  ///< Human-readable explanation.
    QString path;                                     ///< Field/entity path, e.g. "pins[2].pin_number".
};

/// What actually happened to the candidate.
enum class ImportOutcome {
    Imported,   ///< Stored as a new immutable DRAFT version.
    Skipped,    ///< Already present (same scope/component/version); never overwritten.
    Rejected,   ///< Blocked by an ERROR-severity rule; nothing stored.
};

constexpr const char* importOutcomeName(ImportOutcome o) noexcept
{
    switch (o) {
    case ImportOutcome::Imported: return "IMPORTED";
    case ImportOutcome::Skipped:  return "SKIPPED";
    case ImportOutcome::Rejected: return "REJECTED";
    }
    return "REJECTED";
}

struct ImportReport final
{
    ImportOutcome outcome = ImportOutcome::Rejected;
    CatalogScope scope = CatalogScope::User;
    core::ComponentId componentId;
    core::ComponentVersionId versionId;
    QString contentHash;
    QString sourceReference;
    std::vector<ImportMessage> messages;

    void add(QString ruleId, ImportSeverity severity, QString message, QString path = {})
    {
        messages.push_back(ImportMessage{std::move(ruleId), severity, std::move(message), std::move(path)});
    }

    [[nodiscard]] bool hasErrors() const noexcept
    {
        for (const ImportMessage& m : messages) {
            if (m.severity == ImportSeverity::Error) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] int warningCount() const noexcept
    {
        int n = 0;
        for (const ImportMessage& m : messages) {
            if (m.severity == ImportSeverity::Warning) {
                ++n;
            }
        }
        return n;
    }

    /// One-line summary for the status bar / dialog.
    [[nodiscard]] QString summary() const
    {
        return QStringLiteral("%1 %2 (%3 error%4, %5 warning%6)")
            .arg(QString::fromLatin1(importOutcomeName(outcome)),
                 componentId.isEmpty() ? QStringLiteral("component") : componentId.value())
            .arg(hasErrors() ? QStringLiteral("with issues") : QStringLiteral("cleanly"))
            .arg(QString())
            .arg(warningCount())
            .arg(warningCount() == 1 ? QString() : QStringLiteral("s"));
    }
};

} // namespace ardulab::components
