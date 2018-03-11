#include <QDebug>
#include <QQuickView>
#include <QGuiApplication>

#include <sailfishapp.h>

#include "VMQuickVodDownloadModel.h"

#define NAMESPACE "org.duckdns.jgressmann"

int main(int argc, char *argv[])
{
    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));
    QScopedPointer<QQuickView> view(SailfishApp::createView());

    qmlRegisterType<VMQuickVodDownloadModel>(NAMESPACE, 1, 0, "VodDownloadModel");
//    qmlRegisterUncreatableType<VMVodFileDownload>(NAMESPACE, 1, 0, "VMVodFileDownload", QStringLiteral("Q_GADGET"));
//    qmlRegisterUncreatableType<VMVodDescription>(NAMESPACE, 1, 0, "VMVodDescription", QStringLiteral("VMVodDescription"));
//    qmlRegisterUncreatableType<VMVodFormat>(NAMESPACE, 1, 0, "VMVodFormat", QStringLiteral("VMVodFormat"));
//    qmlRegisterUncreatableType<VMVod>(NAMESPACE, 1, 0, "VMVod", QStringLiteral("VMVod"));
    qmlRegisterUncreatableType<VMVodEnums>(NAMESPACE, 1, 0, "VM", QStringLiteral("wrapper around C++ enums"));

    view->setSource(SailfishApp::pathToMainQml());
    view->requestActivate();
    view->show();
    return app->exec();
}
