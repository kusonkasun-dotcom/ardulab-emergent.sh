#include "connection/NetIdentity.h"

#include <QCryptographicHash>

#include <algorithm>


namespace ardulab::connection {


core::NetId NetIdentity::generate(
    const NetIdentityInput& input
    )
{
    const QString normalized = normalize(input);


    const QByteArray hash =
        QCryptographicHash::hash(
            normalized.toUtf8(),
            QCryptographicHash::Sha256
            );


    /*
     * Use first 8 hexadecimal characters.
     *
     * Example:
     * A83F91D2
     *
     * Full SHA256 is unnecessary for human-readable
     * engineering identifiers.
     */
    const QString shortHash =
        QString::fromLatin1(hash.toHex().left(8))
            .toUpper();


    return core::NetId(
        QStringLiteral("NET-") + shortHash
        );
}


QString NetIdentity::normalize(
    const NetIdentityInput& input
    )
{
    std::vector<QString> sortedKeys =
        input.connectionKeys;


    /*
     * Electrical connectivity is order independent.
     *
     * These:
     *
     * U001.VDD
     * C101.1
     * C102.1
     *
     * and:
     *
     * C102.1
     * U001.VDD
     * C101.1
     *
     * represent the same electrical net.
     */
    std::sort(
        sortedKeys.begin(),
        sortedKeys.end()
        );


    QString result;


    for (const QString& key : sortedKeys)
    {
        if (!result.isEmpty())
        {
            result += QStringLiteral("|");
        }

        result += key;
    }


    return result;
}


} // namespace ardulab::connection