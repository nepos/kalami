#ifndef TYPES_H
#define TYPES_H

#include <QMap>
#include <QVariant>

typedef QMap<QString, QVariantMap> StringVariantMap;
typedef QMap<QString, QByteArray> StringByteArrayMap;
typedef QList<uint> UnsignedIntList;

Q_DECLARE_METATYPE(StringVariantMap)
Q_DECLARE_METATYPE(StringByteArrayMap)
Q_DECLARE_METATYPE(UnsignedIntList)

#endif // TYPES_H
