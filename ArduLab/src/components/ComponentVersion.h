#pragma once

// ComponentVersion — immutable catalog revision metadata (Plan §4.5, ADR §3.3).

#include "core/Identifiers.h"

#include <QDateTime>
#include <QString>

namespace ardulab::components {

enum class SourceType {
    Manual,
    JsonImport,
    LegacyImport,
    AiProposal,
    Community,
};

enum class ValidationState {
    NotValidated,
    Passed,
    PassedWithWarnings,
    Failed,
};

constexpr const char* sourceTypeName(SourceType s) noexcept
{
    switch (s) {
    case SourceType::Manual:       return "MANUAL";
    case SourceType::JsonImport:   return "JSON_IMPORT";
    case SourceType::LegacyImport: return "LEGACY_IMPORT";
    case SourceType::AiProposal:   return "AI_PROPOSAL";
    case SourceType::Community:    return "COMMUNITY";
    }
    return "MANUAL";
}

constexpr const char* validationStateName(ValidationState v) noexcept
{
    switch (v) {
    case ValidationState::NotValidated:       return "NOT_VALIDATED";
    case ValidationState::Passed:             return "PASSED";
    case ValidationState::PassedWithWarnings: return "PASSED_WITH_WARNINGS";
    case ValidationState::Failed:             return "FAILED";
    }
    return "NOT_VALIDATED";
}

/// Current approved component data-contract version (ADR §6.2).
inline const QString kCurrentComponentSchemaVersion = QStringLiteral("1.0");

struct ComponentVersion final
{
    core::ComponentVersionId versionId;   ///< "<component_id>@<version>"
    core::ComponentId componentId;
    QString version;                      ///< Semantic version, e.g. "1.0.0".
    QString componentSchemaVersion = kCurrentComponentSchemaVersion;
    QString contentHash;                  ///< "sha256:..." canonical payload hash.
    SourceType sourceType = SourceType::Manual;
    QString sourceReference;              ///< File, URL, or provenance note.
    ValidationState validationState = ValidationState::NotValidated;
    QDateTime createdUtc;
    QDateTime activatedUtc;               ///< Null until activated.
    QString createdBy;
};

} // namespace ardulab::components
