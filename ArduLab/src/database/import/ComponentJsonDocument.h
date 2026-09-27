#pragma once

// ComponentJsonDocument — parsed transport model for a component import.
//
// Holds the raw bytes (provenance), the detected schema versions, and the
// root JSON object. Kept separate from the domain Component so unknown/raw
// fields can be preserved for diagnostics.

#include "database/import/ComponentJsonSchema.h"

#include <QByteArray>
#include <QJsonObject>
#include <QString>

namespace ardulab::database {

struct ComponentJsonDocument final
{
    QByteArray raw;
    QString sourceReference;
    QJsonObject root;
    SchemaDetection schema;
};

} // namespace ardulab::database
