#include "misc.h"
//#include <QDBusMetaType>
//#include <QDBusArgument>
//#include <QDataStream>

//namespace {
//int QUrlRegistered = qDBusRegisterMetaType<QUrl>();
//int QUrlRegistered2 = qRegisterMetaType<QUrl>();
//}

//QDBusArgument& operator<<(QDBusArgument& arg, const QUrl& url) {
//    arg.beginStructure();
//    arg << url.toEncoded();
//    arg.endStructure();
//    return arg;
//}

//const QDBusArgument& operator>>(const QDBusArgument& arg, QUrl& url) {
//    QByteArray s;
//    arg.beginStructure();
//    arg >> s;
//    arg.endStructure();
//    url = QUrl::fromEncoded(s);
//    return arg;
//}



//QDBusArgument& operator<<(QDBusArgument& arg, float f) {
//    arg << (double)f;
//    return arg;
//}

//const QDBusArgument& operator>>(const QDBusArgument& arg, float& f) {
//    double d;
//    arg >> d;
//    f = (float)d;
//    return arg;
//}
