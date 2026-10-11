#pragma once

#include <QString>
#include <vector>


namespace ardulab::connection {


enum class ValidationSeverity
{
    Info,
    Warning,
    Error
};



struct ValidationIssue
{
    ValidationSeverity severity;

    QString code;

    QString message;
};



class ValidationResult final
{
public:

    void addIssue(
        const ValidationIssue& issue
        );


    bool hasErrors() const noexcept;


    bool isValid() const noexcept;


    const std::vector<ValidationIssue>& issues() const noexcept;


private:

    std::vector<ValidationIssue> m_issues;

};


}