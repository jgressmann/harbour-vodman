/* The MIT License (MIT)
 *
 * Copyright (c) 2018 Jean Gressmann <jean@0x42.de>
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

#include "VMQuickVodDownloadModel.h"
#include "VMVodMetaDataDownload.h"
#include "VMQuickVodDownload.h"

#include <QDBusConnection>
#include <QDebug>
#include <QDataStream>
#include <QDBusPendingCallWatcher>
#include <QUrl>
#include <QNetworkConfiguration>

static
QHash<int, QByteArray>
MakeRoleNames() {
    QHash<int, QByteArray> roleNames;
    roleNames.insert(Qt::UserRole + 1, "download");
    return roleNames;
}

const QHash<int, QByteArray> VMQuickVodDownloadModel::ms_Roles = MakeRoleNames();

VMQuickVodDownloadModel::~VMQuickVodDownloadModel()
{
    const auto beg = m_Downloads.begin();
    const auto end = m_Downloads.end();
    for (auto it = beg; it != end; ++it) {
        delete it.value();
    }
}

VMQuickVodDownloadModel::VMQuickVodDownloadModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_Lock(QMutex::Recursive)
    , m_Token(-1)
{
    QDBusConnection connection = QDBusConnection::sessionBus();
    m_Service = new org::duckdns::jgressmann::vodman::service("org.duckdns.jgressmann.vodman.service", "/instance", connection);
    m_Service->setParent(this);

    connect(
                m_Service,
                &org::duckdns::jgressmann::vodman::service::vodFileDownloadRemoved,
                this,
                &VMQuickVodDownloadModel::onVodFileDownloadRemoved);

    connect(
                m_Service,
                &org::duckdns::jgressmann::vodman::service::vodFileDownloadChanged,
                this,
                &VMQuickVodDownloadModel::onVodFileDownloadChanged);




    connect(
                m_Service,
                &org::duckdns::jgressmann::vodman::service::vodMetaDataDownloadCompleted,
                this,
                &VMQuickVodDownloadModel::onVodFileMetaDataDownloadCompleted);


    connect(&m_NetworkConfigurationManager, &QNetworkConfigurationManager::onlineStateChanged, this, &VMQuickVodDownloadModel::onOnlineChanged);
}

void
VMQuickVodDownloadModel::onVodFileDownloadAdded(qint64 handle, const QByteArray& result)
{
    qDebug() << "enter" << handle;
    VMVodFileDownload download;
    {
        QDataStream s(result);
        s >> download;
    }

    if (download.isValid()) {
        QMutexLocker g(&m_Lock);
        if (m_Downloads.count(handle)) {
            qWarning() << "token" << handle << "already added to downloads";
        } else {
            vodFileDownloadAdded(handle, download);
        }
    }

    qDebug() << "exit" << handle;
}

void
VMQuickVodDownloadModel::onVodFileDownloadRemoved(qint64 handle, const QByteArray& result)
{
    qDebug() << "enter" << handle;
    QMutexLocker g(&m_Lock);
    auto it = m_Downloads.find(handle);
    if (m_Downloads.end() == it) {
        qDebug() << "received download removed event for handle" << handle
                 << "but no such entry.";
    } else {
        it.value()->deleteLater();
        m_Downloads.erase(it);
        auto row = getHandleRow(handle);
        beginRemoveRows(QModelIndex(), row, row);
        m_Rows.removeAt(row);
        endRemoveRows();
        qDebug() << "removed row" << row;
    }

    if (m_UserDownloads.removeOne(handle)) {
        VMVodFileDownload download;
        {
            QDataStream s(result);
            s >> download;
        }

        qDebug() << "mine removed" << handle << download;

        if (download.isValid()) {
            emit downloadSucceeded(QVariant::fromValue(download));
        } else {
            emit downloadFailed(
                        download.data().description.webPageUrl(),
                        download.error(),
                        download.data()._filePath);
        }
    }

    qDebug() << "exit" << handle;
}

void
VMQuickVodDownloadModel::onVodFileDownloadChanged(qint64 handle, const QByteArray& result)
{
    qDebug() << "enter" << handle;
    VMVodFileDownload download;
    {
        QDataStream s(result);
        s >> download;
    }

    if (!download.isValid()) {
        qDebug() << "exit" << handle << download;
        return;
    }

    QMutexLocker g(&m_Lock);
    auto it = m_Downloads.find(handle);
    if (m_Downloads.end() == it) {
        vodFileDownloadAdded(handle, download);
        return;
    }

    qDebug() << "changed handle" << handle << "download" << download;
    it.value()->setData(download);
    auto row = getHandleRow(handle);
    emit dataChanged(index(row), index(row));
    qDebug() << "exit" << handle;
    return;

//    QVector<int> roles;
//    roles.reserve(ms_Roles.size() + 1);
//    roles << Qt::DisplayRole;
//    const auto beg = ms_Roles.keyBegin();
//    const auto end = ms_Roles.keyEnd();
//    for (auto it = beg; it != end; ++it) {
//        roles << *it;
//    }

//    auto row = getHandleRow(handle);
//    //emit dataChanged(createIndex(row, 0), createIndex(row, 0), roles);
//    //emit dataChanged(index(row), index(row));
//    //emit dataChanged(index(row), index(row), roles);
//    emit beginResetModel();
//    emit endResetModel();
//    qDebug() << "exit" << handle;
}

void
VMQuickVodDownloadModel::onVodFileMetaDataDownloadCompleted(qint64 token, const QByteArray& result)
{
    qDebug() << "enter" << token;
    QMutexLocker g(&m_Lock);
    if (token == m_Token) {
        m_UserDownloads << token;
        auto url = m_Url;
        m_Token = -1;
        m_Url.clear();
        emit canStartDownloadChanged();

        VMVodMetaDataDownload download;
        {
            QDataStream s(result);
            s >> download;
        }

        qDebug() << "mine" << token << download;

        if (download.isValid()) {
            if (download.error() == VMVodEnums::VM_ErrorNone) {
                emit metaDataDownloadSucceeded(token, download.vod());
            } else {
                emit downloadFailed(url, download.error(), QString());
            }
        } else {
            if (download.error() != VMVodEnums::VM_ErrorNone) {
                emit downloadFailed(url, download.error(), QString());
            } else {
                emit downloadFailed(url, VMVodEnums::VM_ErrorUnknown, QString());
            }
        }
    } else {
        // nop
    }

    qDebug() << "exit" << token;
}

int
VMQuickVodDownloadModel::getHandleRow(qint64 handle) const
{
    return m_Rows.indexOf(handle);
}

QVariant
VMQuickVodDownloadModel::data(const QModelIndex &index, int role) const
{
    if (role > Qt::UserRole) {
        int column = role - Qt::UserRole - 1;

        QMutexLocker g(&m_Lock);
        if (index.isValid() &&
                index.row() < m_Rows.size()) {
            auto handle = m_Rows[index.row()];
            const auto& download = m_Downloads[handle];
            switch (column) {
            case 0:
                return QVariant::fromValue(download);
            }
        }
    }

    return QVariant();
}

int
VMQuickVodDownloadModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    QMutexLocker g(&m_Lock);
    return m_Rows.size();
}


void
VMQuickVodDownloadModel::startDownloadMetaData(const QString& url)
{
    QMutexLocker g(&m_Lock);
    if (!canStartDownload()) {
        return;
    }

    m_Url = url;
    auto reply = m_Service->newToken();
    auto watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, &VMQuickVodDownloadModel::onNewTokenReply);
    emit canStartDownloadChanged();
}

void
VMQuickVodDownloadModel::cancelDownloadMetaData()
{
    QMutexLocker g(&m_Lock);
    m_Url.clear();
    m_Token = -1;
    emit canStartDownloadChanged();
}

void
VMQuickVodDownloadModel::startDownloadVod(
        qint64 token, const VMVod& vod, int formatIndex, const QString& filePath) {

    QMutexLocker g(&m_Lock);
    qDebug() << "token" << token << "vod" << vod << "index" << formatIndex << "path" << filePath;

    if (!vod.isValid() || formatIndex < 0 || formatIndex >= vod.formats() ||
        filePath.isEmpty()) {
        qDebug() << "invalid params";
        return;
    }


    VMVodFileDownloadRequest req;
    req.description = vod.data().description;
    req.filePath = filePath;
    req.format = vod.data()._formats[formatIndex];
    QByteArray b;
    {
        QDataStream s(&b, QIODevice::WriteOnly);
        s << req;
    }

    auto pendingReply = m_Service->startFetchVodFile(token, b);
    auto watcher = new QDBusPendingCallWatcher(pendingReply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, &VMQuickVodDownloadModel::onStartDownloadVodFileReply);
}


void
VMQuickVodDownloadModel::onNewTokenReply(QDBusPendingCallWatcher *self) {
    QMutexLocker g(&m_Lock);
    self->deleteLater();
    QDBusPendingReply<qint64> reply = *self;
    if (reply.isValid()) {
        m_Token = reply.value();
        auto reply = m_Service->startFetchVodMetaData(m_Token, m_Url);
        auto watcher = new QDBusPendingCallWatcher(reply, this);
        connect(watcher, &QDBusPendingCallWatcher::finished, this, &VMQuickVodDownloadModel::onMetaDataDownloadReply);
//        auto token = reply.value();
//        if (token >= 0) {
//            m_Token = token;
//            auto reply = m_Service->startFetchVodMetaData(m_Token, m_Url);
//            auto watcher = new QDBusPendingCallWatcher(reply, this);
//            connect(watcher, &QDBusPendingCallWatcher::finished, this, &VMQuickVodDownloadModel::onMetaDataDownloadReply);
//        } else {
//            // wtf, service doesn't work right, happens if
//            // the generated D-Bus adaptor code can't call the slot? in owning class
//            qDebug() << "invalid token received" << token;
//            emit downloadFailed(m_Url, VMVodEnums::VM_ErrorServiceUnavailable, QString());
//            m_Url.clear();
//            emit canStartDownloadChanged();
//        }
    } else {
         qDebug() << "invalid new token reply" << reply.error();
         emit downloadFailed(m_Url, VMVodEnums::VM_ErrorServiceUnavailable, QString());
         m_Url.clear();
         emit canStartDownloadChanged();
    }
}

void
VMQuickVodDownloadModel::onMetaDataDownloadReply(QDBusPendingCallWatcher *self) {
    QMutexLocker g(&m_Lock);
    self->deleteLater();
    QDBusPendingReply<> reply = *self;
    if (reply.isValid()) {
        // nothing to do
    } else {
        qDebug() << "invalid metadata download reply for" << m_Url  << "error" << reply.error();
        m_Token = -1;
        emit downloadFailed(m_Url, VMVodEnums::VM_ErrorServiceUnavailable, QString());
        m_Url.clear();
        emit canStartDownloadChanged();
    }
}

void
VMQuickVodDownloadModel::onStartDownloadVodFileReply(QDBusPendingCallWatcher *self) {
    QMutexLocker g(&m_Lock);
    self->deleteLater();
    QDBusPendingReply<> reply = *self;
    if (reply.isValid()) {
        // nothing to do
    } else {
        qDebug() << "invalid reply" << reply.error();
        m_Token = -1;
        emit downloadFailed(m_Url, VMVodEnums::VM_ErrorServiceUnavailable, QString());
        m_Url.clear();
        emit canStartDownloadChanged();
    }
}

void
VMQuickVodDownloadModel::cancelDownload(int index, bool deleteFile)
{
    qDebug() << "index" << index << "delete?" << deleteFile;
    QMutexLocker g(&m_Lock);
    if (index < m_Rows.size()) {
        auto handle = m_Rows[index];
        qDebug() << "handle" << handle << "delete?" << deleteFile;
        m_Service->cancelFetchVodFile(handle, deleteFile);
    }
}

void
VMQuickVodDownloadModel::cancelDownloads(bool deleteFiles) {
    qDebug() << "delete?" << deleteFiles;
    QMutexLocker g(&m_Lock);
    for (auto i = 0; i < m_Rows.size(); ++i) {
        auto handle = m_Rows[i];
        qDebug() << "cancel" << handle;
        m_Service->cancelFetchVodFile(handle, deleteFiles);
    }
}

void
VMQuickVodDownloadModel::vodFileDownloadAdded(qint64 handle, const VMVodFileDownload& download) {
    auto it = m_Downloads.insert(handle, new VMQuickVodDownload(this));
    it.value()->setData(download);
    beginInsertRows(QModelIndex(), m_Rows.count(), m_Rows.count());
    m_Rows << handle;
    endInsertRows();
    qDebug() << "added row" << m_Rows.count();
}

QHash<int, QByteArray>
VMQuickVodDownloadModel::roleNames() const {
    return ms_Roles;
}

bool
VMQuickVodDownloadModel::canStartDownload() const {
    return m_Url.isEmpty();
}

QString
VMQuickVodDownloadModel::sanatizePath(QString path) const {
    // https://msdn.microsoft.com/en-us/library/aa365247
    static const QRegExp s_WindowsForbidden("[<>:\"\\|?*]");
    Q_ASSERT(s_WindowsForbidden.isValid());

    path.replace(s_WindowsForbidden, QString());

    auto index = path.lastIndexOf(QChar('/'));
    if (index >= 0) {
        const int MaxLen = 250;
        if (path.size() - index - 1 > MaxLen) { // need to truncate
            auto dir = path.left(index + 1);
            auto file = path.right(path.size() - index - 1);
            index = file.lastIndexOf(QChar('.'));
            if (index > 0) { // has extension
                auto name = file.left(index);
                auto extension = file.right(file.size()-index);
                // well formed? (small extension large name
                if (name.size() > extension.size()) {
                    name.resize(MaxLen - extension.size());
                }

                file = name + extension;
                if (file.size() > MaxLen) { // large extension small name
                    // ouch
                    file.resize(MaxLen);
                }
            } else {
                file.resize(MaxLen);
            }

            path = dir + file;
        }
    } else {
        qWarning("path %s is not absolute\n", qPrintable(path));
    }

    return path;
}

bool
VMQuickVodDownloadModel::isUrl(QString str) const {
    QUrl url(str);
    return url.isValid();
}

bool
VMQuickVodDownloadModel::isOnline() const {
    return m_NetworkConfigurationManager.isOnline();
}

bool
VMQuickVodDownloadModel::isOnBroadband() const {
    auto configs = m_NetworkConfigurationManager.allConfigurations(QNetworkConfiguration::Active);
    foreach (const auto& config, configs) {
        if (config.isValid()) {
            switch (config.bearerTypeFamily()) {
            case QNetworkConfiguration::BearerEthernet:
            case QNetworkConfiguration::BearerWLAN:
                return true;
            default:
                break;
            }
        }
    }

    return false;
}

bool
VMQuickVodDownloadModel::isOnMobile() const {
    auto configs = m_NetworkConfigurationManager.allConfigurations(QNetworkConfiguration::Active);
    foreach (const auto& config, configs) {
        if (config.isValid()) {
            switch (config.bearerTypeFamily()) {
            case QNetworkConfiguration::Bearer2G:
            case QNetworkConfiguration::Bearer3G:
            case QNetworkConfiguration::Bearer4G:
                return true;
            default:
                break;
            }
        }
    }

    return false;
}

void
VMQuickVodDownloadModel::onOnlineChanged(bool online) {
    Q_UNUSED(online);
    emit isOnlineChanged();
    emit isOnMobileChanged();
    emit isOnBroadbandChanged();
    emit canStartDownloadChanged();
}
