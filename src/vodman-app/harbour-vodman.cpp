#include <QDebug>
#include <QQuickView>
#include <QGuiApplication>

#include <sailfishapp.h>

#include "VMQuickVodDownloadModel.h"

#define NAMESPACE "org.duckdns.jgressmann"

int main(int argc, char *argv[])
{
    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));
    app->setApplicationVersion(QStringLiteral("%1.%2.%3").arg(QString::number(VODMAN_VERSION_MAJOR), QString::number(VODMAN_VERSION_MINOR), QString::number(VODMAN_VERSION_PATCH)));

    qDebug() << app->applicationName()
             << "version" << app->applicationVersion();


    qmlRegisterType<VMQuickVodDownloadModel>(NAMESPACE, 1, 0, "VodDownloadModel");
//    qmlRegisterUncreatableType<VMVodFileDownload>(NAMESPACE, 1, 0, "VMVodFileDownload", QStringLiteral("Q_GADGET"));
//    qmlRegisterUncreatableType<VMVodDescription>(NAMESPACE, 1, 0, "VMVodDescription", QStringLiteral("VMVodDescription"));
//    qmlRegisterUncreatableType<VMVodFormat>(NAMESPACE, 1, 0, "VMVodFormat", QStringLiteral("VMVodFormat"));
//    qmlRegisterUncreatableType<VMVod>(NAMESPACE, 1, 0, "VMVod", QStringLiteral("VMVod"));
    qmlRegisterUncreatableType<VMVodEnums>(NAMESPACE, 1, 0, "VM", QStringLiteral("wrapper around C++ enums"));

    QScopedPointer<QQuickView> view(SailfishApp::createView());
    view->setSource(SailfishApp::pathToMainQml());
    view->requestActivate();
    view->show();
    return app->exec();
}
