#include "connection/ValidationResult.h"


namespace ardulab::connection {


void ValidationResult::addIssue(
    const ValidationIssue& issue
    )
{
    m_issues.push_back(issue);
}



bool ValidationResult::hasErrors() const noexcept
{
    for (const auto& issue : m_issues)
    {
        if (issue.severity == ValidationSeverity::Error)
        {
            return true;
        }
    }

    return false;
}



bool ValidationResult::isValid() const noexcept
{
    return !hasErrors();
}



const std::vector<ValidationIssue>&
ValidationResult::issues() const noexcept
{
    return m_issues;
}


} // namespace ardulab::connection