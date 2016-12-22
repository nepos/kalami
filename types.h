#ifndef TYPES_H
#define TYPES_H

#include <QMap>
#include <QVariant>

typedef QMap<QString, QVariantMap> StringVariantMap;
typedef QMap<QString, QByteArray> StringByteArrayMap;


Q_DECLARE_METATYPE(StringVariantMap)
Q_DECLARE_METATYPE(StringByteArrayMap)

#endif // TYPES_H
