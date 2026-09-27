#pragma once

// Core error contract (Plan §4.2).
//
// Carries a stable error code, a human-readable message, and optional context.
// Domain and UI interfaces receive an Error — never a raw Qt SQL error, a
// QJsonParseError, or a C++ exception from a lower layer.

#include <QString>

namespace ardulab::core {

enum class ErrorCode {
    None = 0,

    // Generic
    InvalidArgument,
    NotFound,
    AlreadyExists,
    NotSupported,
    InvalidState,
    Internal,

    // I/O and format
    IoFailure,
    ParseFailure,
    UnsupportedFormatVersion,

    // Catalog / component domain
    CatalogUnavailable,
    ComponentNotFound,
    ComponentVersionNotFound,
    ValidationFailed,

    // Project domain
    ProjectNotOpen,
    ProjectAlreadyOpen,
    UnresolvedReference,
};

/// Stable textual form of an ErrorCode for logs, reports, and tests.
constexpr const char* errorCodeName(ErrorCode code) noexcept
{
    switch (code) {
    case ErrorCode::None:                     return "NONE";
    case ErrorCode::InvalidArgument:          return "INVALID_ARGUMENT";
    case ErrorCode::NotFound:                 return "NOT_FOUND";
    case ErrorCode::AlreadyExists:            return "ALREADY_EXISTS";
    case ErrorCode::NotSupported:             return "NOT_SUPPORTED";
    case ErrorCode::InvalidState:             return "INVALID_STATE";
    case ErrorCode::Internal:                 return "INTERNAL";
    case ErrorCode::IoFailure:                return "IO_FAILURE";
    case ErrorCode::ParseFailure:             return "PARSE_FAILURE";
    case ErrorCode::UnsupportedFormatVersion: return "UNSUPPORTED_FORMAT_VERSION";
    case ErrorCode::CatalogUnavailable:       return "CATALOG_UNAVAILABLE";
    case ErrorCode::ComponentNotFound:        return "COMPONENT_NOT_FOUND";
    case ErrorCode::ComponentVersionNotFound: return "COMPONENT_VERSION_NOT_FOUND";
    case ErrorCode::ValidationFailed:         return "VALIDATION_FAILED";
    case ErrorCode::ProjectNotOpen:           return "PROJECT_NOT_OPEN";
    case ErrorCode::ProjectAlreadyOpen:       return "PROJECT_ALREADY_OPEN";
    case ErrorCode::UnresolvedReference:      return "UNRESOLVED_REFERENCE";
    }
    return "UNKNOWN";
}

class Error final
{
public:
    Error() = default;

    Error(ErrorCode code, QString message, QString context = {})
        : m_code(code)
        , m_message(std::move(message))
        , m_context(std::move(context))
    {
    }

    [[nodiscard]] ErrorCode code() const noexcept { return m_code; }
    [[nodiscard]] const QString& message() const noexcept { return m_message; }
    [[nodiscard]] const QString& context() const noexcept { return m_context; }
    [[nodiscard]] bool isNone() const noexcept { return m_code == ErrorCode::None; }

    /// "CODE: message [context]" — for diagnostics and status bars.
    [[nodiscard]] QString toString() const
    {
        QString text = QString::fromLatin1(errorCodeName(m_code));
        if (!m_message.isEmpty()) {
            text += QStringLiteral(": ") + m_message;
        }
        if (!m_context.isEmpty()) {
            text += QStringLiteral(" [") + m_context + QLatin1Char(']');
        }
        return text;
    }

    friend bool operator==(const Error& a, const Error& b) noexcept
    {
        return a.m_code == b.m_code && a.m_message == b.m_message && a.m_context == b.m_context;
    }
    friend bool operator!=(const Error& a, const Error& b) noexcept { return !(a == b); }

private:
    ErrorCode m_code = ErrorCode::None;
    QString m_message;
    QString m_context;
};

} // namespace ardulab::core
