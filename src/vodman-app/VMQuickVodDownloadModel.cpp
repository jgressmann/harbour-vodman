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

#include "VMQuickVodDownloadModel.h"
#include "VMVodMetaDataDownload.h"
#include "VMQuickVodDownload.h"

#include <QDebug>
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
    , m_Ytdl(this)
    , m_Token(-1)
    , m_TokenGenerator(0)
{
    connect(&m_Ytdl, &VMYTDL::ytdlPathChanged, this, &VMQuickVodDownloadModel::onYtdlPathChanged);
    connect(&m_Ytdl, &VMYTDL::vodFileDownloadCompleted, this, &VMQuickVodDownloadModel::onVodFileDownloadCompleted);
    connect(&m_Ytdl, &VMYTDL::vodFileDownloadChanged, this, &VMQuickVodDownloadModel::onVodFileDownloadChanged);
    connect(&m_Ytdl, &VMYTDL::vodMetaDataDownloadCompleted, this, &VMQuickVodDownloadModel::onVodFileMetaDataDownloadCompleted);

    connect(&m_NetworkConfigurationManager, &QNetworkConfigurationManager::onlineStateChanged, this, &VMQuickVodDownloadModel::onOnlineChanged);
}

void
VMQuickVodDownloadModel::onVodFileDownloadCompleted(qint64 handle, const VMVodFileDownload& download)
{
    qDebug() << "enter" << handle;
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
        if (m_UserDownloadsFilePaths.removeOne(download.filePath())) {
            emit downloadsPendingChanged();
            qDebug() << "file path" << download.filePath() << "removed from user downloads";
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
VMQuickVodDownloadModel::onVodFileDownloadChanged(qint64 handle, const VMVodFileDownload& download)
{
    qDebug() << "enter" << handle;

    if (!download.isValid()) {
        qDebug() << "exit" << handle << download;
        return;
    }

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
}

void
VMQuickVodDownloadModel::onVodFileMetaDataDownloadCompleted(qint64 token, const VMVodMetaDataDownload& download)
{
    qDebug() << "enter" << token;
    if (token == m_Token) {
        m_UserDownloads << token;
        auto url = m_Url;
        m_Token = -1;
        m_Url.clear();
        emit canStartDownloadChanged();

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
    return m_Rows.size();
}


void
VMQuickVodDownloadModel::startDownloadMetaData(const QString& url)
{
    if (!canStartDownload()) {
        return;
    }

    if (url.isEmpty()) {
        qDebug() << "empty url";
        return;
    }

    m_Url = url;
    m_Token = m_TokenGenerator++;
    m_Ytdl.startFetchVodMetaData(m_Token, m_Url);
    emit canStartDownloadChanged();
}

void
VMQuickVodDownloadModel::cancelDownloadMetaData()
{
    m_Url.clear();
    m_Token = -1;
    emit canStartDownloadChanged();
}

void
VMQuickVodDownloadModel::startDownloadVod(
        qint64 token, const VMVod& vod, int formatIndex, const QString& filePath) {

    if (!canStartDownload()) {
        return;
    }

    qDebug() << "token" << token << "vod" << vod << "index" << formatIndex << "path" << filePath;

    if (!vod.isValid() || formatIndex < 0 || formatIndex >= vod.formats() ||
        filePath.isEmpty()) {
        qDebug() << "invalid params";
        return;
    }

    if (m_UserDownloadsFilePaths.indexOf(filePath) >= 0) {
        emit downloadFailed(vod.description().webPageUrl(), VMVodEnums::VM_ErrorAlreadyDownloading, filePath);
        return;
    }

    m_FilePath = filePath;

    VMVodFileDownloadRequest req;
    req.description = vod.data().description;
    req.filePath = filePath;
    req.format = vod.data()._formats[formatIndex];

    m_UserDownloadsFilePaths << m_FilePath;
    emit downloadsPendingChanged();
    m_FilePath.clear();
    m_Ytdl.startFetchVodFile(token, req);
}

void
VMQuickVodDownloadModel::cancelDownload(int index, bool deleteFile)
{
    qDebug() << "index" << index << "delete?" << deleteFile;

    if (index < m_Rows.size()) {
        auto handle = m_Rows[index];
        qDebug() << "handle" << handle << "delete?" << deleteFile;
        m_Ytdl.cancelFetchVodFile(handle, deleteFile);
    }
}

void
VMQuickVodDownloadModel::cancelDownloads(bool deleteFiles) {
    qDebug() << "delete?" << deleteFiles;

    for (auto i = 0; i < m_Rows.size(); ++i) {
        auto handle = m_Rows[i];
        qDebug() << "cancel" << handle;
        m_Ytdl.cancelFetchVodFile(handle, deleteFiles);
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
    return m_Url.isEmpty() && m_FilePath.isEmpty();
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
            case QNetworkConfiguration::BearerLTE:
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

bool
VMQuickVodDownloadModel::downloadsPending() const
{
    return !m_UserDownloadsFilePaths.isEmpty();
}

QString
VMQuickVodDownloadModel::ytdlPath() const
{
    return m_Ytdl.ytdlPath();
}

void
VMQuickVodDownloadModel::setYtdlPath(const QString& path)
{
    m_Ytdl.setYtdlPath(path);
}

void
VMQuickVodDownloadModel::onYtdlPathChanged()
{
    emit ytdlPathChanged();
}
