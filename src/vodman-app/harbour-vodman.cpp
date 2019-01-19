/* The MIT License (MIT)
 *
 * Copyright (c) 2018, 2019 Jean Gressmann <jean@0x42.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <QDebug>
#include <QQuickView>
#include <QGuiApplication>
#include <QQmlEngine>
#include <QStandardPaths>

#include <sailfishapp.h>

#include "VMQuickVodDownloadModel.h"
#include "VMApp.h"
#include "VMYTDL.h"

static QObject *singletonTypeProvider(QQmlEngine *, QJSEngine *)
{
    return new VMApp();
}

int main(int argc, char *argv[])
{
    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));
    app->setApplicationVersion(QStringLiteral("%1.%2.%3").arg(QString::number(VODMAN_VERSION_MAJOR), QString::number(VODMAN_VERSION_MINOR), QString::number(VODMAN_VERSION_PATCH)));

    qInfo("%s version %s\n", qPrintable(app->applicationName()), qPrintable(app->applicationVersion()));

    VMYTDL::initialize();

    qmlRegisterType<VMQuickVodDownloadModel>(VODMAN_NAMESPACE, 1, 0, "VodDownloadModel");
    qmlRegisterUncreatableType<VMVodEnums>(VODMAN_NAMESPACE, 1, 0, "VM", QStringLiteral("wrapper around C++ enums"));
    qmlRegisterSingletonType<VMApp>(VODMAN_NAMESPACE, 1, 0, "App", singletonTypeProvider);


    QScopedPointer<QQuickView> view(SailfishApp::createView());
    view->setSource(SailfishApp::pathToMainQml());
    view->requestActivate();
    view->show();
    auto exitCode = app->exec();
    VMYTDL::finalize();
    return exitCode;
}
