#pragma once

#include <QObject>
#include <QUrl>

//class QDBusArgument;
class QDataStream;
//QDBusArgument& operator<<(QDBusArgument& arg, const QUrl& url);
//const QDBusArgument& operator>>(const QDBusArgument& arg, QUrl& url);
//QDataStream& operator<<(QDataStream& arg, const QUrl& url);
//const QDataStream& operator>>(const QDataStream& arg, QUrl& url);

Q_DECLARE_METATYPE(QUrl)

//// floats
//QDBusArgument& operator<<(QDBusArgument& arg, float f);
//const QDBusArgument& operator>>(const QDBusArgument& arg, float& f);

// https://stackoverflow.com/questions/19135101/qtdbus-send-enums-over-dbus
#define DECLARE_ENUM_DATATYPE(ENUM_TYPE_DBUS)\
     QDBusArgument &operator<<(QDBusArgument &argument, ENUM_TYPE_DBUS value);\
     const QDBusArgument &operator>>(const QDBusArgument &argument, ENUM_TYPE_DBUS &val);


#define CREATE_ENUM_DATATYPE(ENUM_TYPE_DBUS)\
     QDBusArgument &operator<<(QDBusArgument &argument, ENUM_TYPE_DBUS value)\
     {\
         int newVal = (int)value;\
         argument << newVal;\
         return argument;\
     }\
     const QDBusArgument &operator>>(const QDBusArgument &argument, ENUM_TYPE_DBUS &val)\
     {\
         int result = 0;\
         argument >> result;\
         val = (ENUM_TYPE_DBUS)result;\
         return argument;\
     }

