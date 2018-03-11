#include <QCoreApplication>
#include <QDebug>
#include <QDBusMetaType>

#include "VMService.h"



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

//QT_BEGIN_NAMESPACE
//Q_DECLARE_METATYPE(QUrl)
//QT_END_NAMESPACE



int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

//    qDBusRegisterMetaType<QUrl>();

    VMYTDL::runInitialCheck();
    if (!VMYTDL::available()) {
        return -1;
    }

    VMService service;

    return app.exec();
}
