#include <QCoreApplication>
#include <QDebug>

#include "VMService.h"


int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    app.setApplicationVersion(QStringLiteral("%1.%2.%3").arg(QString::number(VODMAN_VERSION_MAJOR), QString::number(VODMAN_VERSION_MINOR), QString::number(VODMAN_VERSION_PATCH)));

    qDebug() << app.applicationName()
             << "version" << app.applicationVersion();

    VMYTDL::runInitialCheck();
    VMService service;
    return app.exec();
}
