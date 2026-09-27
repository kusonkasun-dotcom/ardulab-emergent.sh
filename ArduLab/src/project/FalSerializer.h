#pragma once

// FalSerializer — .FAL format adapter (Plan §4.3, Arch Doc §7.3).
//
//   read(bytes)  -> Project + FalReadReport
//   write(Project) -> bytes (current additive format version)
//   canRead(version) -> bool
//
// Compatibility rules (Plan §8.3):
//   - The legacy minimal shape {"project":{name,version},"components":[]} opens.
//   - Absent future sections become empty defaults, never errors.
//   - Unknown root keys and unknown instance fields are preserved.
//   - Instance IDs and millimeter coordinates are preserved exactly.
//   - Catalog references are parsed as-is; resolution (and "unresolved")
//     is decided by ProjectService against the catalog — never here, and
//     never by substituting another version.

#include "core/Result.h"
#include "project/FalDocument.h"
#include "project/Project.h"

#include <QByteArray>

namespace ardulab::project {

struct FalReadResult final
{
    Project project;
    FalReadReport report;
};

class FalSerializer final
{
public:
    [[nodiscard]] static bool canRead(const QString& formatVersion);

    [[nodiscard]] static core::Result<FalDocument> parse(const QByteArray& bytes);

    [[nodiscard]] static core::Result<FalReadResult> read(const QByteArray& bytes);
    [[nodiscard]] static core::Result<FalReadResult> fromDocument(const FalDocument& document);

    [[nodiscard]] static FalDocument toDocument(const Project& project);
    [[nodiscard]] static QByteArray write(const Project& project);
};

} // namespace ardulab::project
