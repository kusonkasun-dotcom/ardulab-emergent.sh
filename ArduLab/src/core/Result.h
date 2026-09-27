#pragma once

// Core result contract (Plan §4.2).
//
// Result<T> is an explicit success/failure return type. It replaces exceptions
// and bool+out-parameter patterns across every module boundary.
//
//   Result<int> r = parse(text);
//   if (!r) { report(r.error()); return; }
//   use(r.value());
//
// Result<void> models an operation with no payload.

#include "core/Error.h"

#include <optional>
#include <utility>

namespace ardulab::core {

template <typename T>
class Result final
{
public:
    static Result success(T value) { return Result(std::move(value)); }
    static Result failure(Error error) { return Result(std::move(error)); }
    static Result failure(ErrorCode code, QString message, QString context = {})
    {
        return Result(Error(code, std::move(message), std::move(context)));
    }

    // Implicit construction from a value keeps call sites terse: `return 42;`
    Result(T value) // NOLINT(google-explicit-constructor)
        : m_value(std::move(value))
    {
    }

    // Implicit construction from an Error: `return Error{...};`
    Result(Error error) // NOLINT(google-explicit-constructor)
        : m_error(std::move(error))
    {
    }

    [[nodiscard]] bool isSuccess() const noexcept { return m_value.has_value(); }
    [[nodiscard]] bool isFailure() const noexcept { return !m_value.has_value(); }
    explicit operator bool() const noexcept { return isSuccess(); }

    /// Precondition: isSuccess().
    [[nodiscard]] const T& value() const& { return *m_value; }
    [[nodiscard]] T& value() & { return *m_value; }
    [[nodiscard]] T&& value() && { return std::move(*m_value); }

    /// Precondition: isFailure(). Returns a None error on success for safety.
    [[nodiscard]] const Error& error() const noexcept { return m_error; }

    [[nodiscard]] T valueOr(T fallback) const
    {
        return m_value.has_value() ? *m_value : std::move(fallback);
    }

private:
    std::optional<T> m_value;
    Error m_error;
};

template <>
class Result<void> final
{
public:
    Result() = default; // success

    Result(Error error) // NOLINT(google-explicit-constructor)
        : m_error(std::move(error))
    {
    }

    static Result success() { return Result(); }
    static Result failure(Error error) { return Result(std::move(error)); }
    static Result failure(ErrorCode code, QString message, QString context = {})
    {
        return Result(Error(code, std::move(message), std::move(context)));
    }

    [[nodiscard]] bool isSuccess() const noexcept { return m_error.isNone(); }
    [[nodiscard]] bool isFailure() const noexcept { return !m_error.isNone(); }
    explicit operator bool() const noexcept { return isSuccess(); }

    [[nodiscard]] const Error& error() const noexcept { return m_error; }

private:
    Error m_error;
};

using Status = Result<void>;

} // namespace ardulab::core
