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

#include "VMQuickVodPlaylistDownloadModel.h"
#include "VMQuickVodPlaylistDownload.h"
#include "VMYTDL.h"

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

const QHash<int, QByteArray> VMQuickVodPlaylistDownloadModel::ms_Roles = MakeRoleNames();

VMQuickVodPlaylistDownloadModel::~VMQuickVodPlaylistDownloadModel()
{
    const auto beg = m_Downloads.begin();
    const auto end = m_Downloads.end();
    for (auto it = beg; it != end; ++it) {
        delete it.value();
    }
}

VMQuickVodPlaylistDownloadModel::VMQuickVodPlaylistDownloadModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_Ytdl(nullptr)
    , m_TokenGenerator(0)
    , m_MetaDataDownloads(0)
{
    connect(&m_NetworkConfigurationManager, &QNetworkConfigurationManager::onlineStateChanged, this, &VMQuickVodPlaylistDownloadModel::onOnlineChanged);
}

void
VMQuickVodPlaylistDownloadModel::onPlaylistDownloadCompleted(qint64 token, const VMPlaylistDownload& download)
{
    m_UrlsBeingDownloaded.remove(token);
    emit busyChanged();

    auto it = m_Downloads.find(token);
    if (m_Downloads.end() == it) {
        qDebug() << "received download removed event for handle" << token
                 << "but no such entry.";
    } else {
        it.value()->deleteLater();
        m_Downloads.erase(it);
        auto row = getHandleRow(token);
        beginRemoveRows(QModelIndex(), row, row);
        m_Rows.removeAt(row);
        endRemoveRows();
        qDebug() << "removed row" << row;
    }

    if (download.isValid()) {
        Q_ASSERT(VMVodEnums::VM_ErrorNone == download.error());
        emit downloadSucceeded(QVariant::fromValue(download));
    } else {
        const auto& downloadData = download.data();
        QString filePath;
        if (downloadData.currentFileIndex >= 0 && downloadData.currentFileIndex < downloadData.files.size()) {
            filePath = downloadData.files[downloadData.currentFileIndex].filePath();
        }

        VMVodEnums::Error error = VMVodEnums::VM_ErrorNone == downloadData.error ? VMVodEnums::VM_ErrorUnknown : downloadData.error;

        emit downloadFailed(downloadData.playlist.webPageUrl(), error, filePath);
    }
}

void
VMQuickVodPlaylistDownloadModel::onPlaylistDownloadChanged(qint64 handle, const VMPlaylistDownload& download)
{
    auto it = m_Downloads.find(handle);
    if (m_Downloads.end() == it) {
        playlistDownloadAdded(handle, download);
        return;
    }

    it.value()->setData(download);
    auto row = getHandleRow(handle);
    emit dataChanged(index(row), index(row));
}

void
VMQuickVodPlaylistDownloadModel::onMetaDataDownloadCompleted(qint64 token, const VMMetaDataDownload& download)
{
    if (--m_MetaDataDownloads == 0) {
        emit metaDataDownloadsPendingChanged();
    }

    emit busyChanged();

    if (download.isValid()) {
        Q_ASSERT(VMVodEnums::VM_ErrorNone == download.error());
        emit metaDataDownloadSucceeded(token, download.playlist());
    } else {
        if (download.error() != VMVodEnums::VM_ErrorNone) {
            emit downloadFailed(download.url(), download.error(), QString());
        } else {
            emit downloadFailed(download.url(), VMVodEnums::VM_ErrorUnknown, QString());
        }
    }
}

int
VMQuickVodPlaylistDownloadModel::getHandleRow(qint64 handle) const
{
    return m_Rows.indexOf(handle);
}

QVariant
VMQuickVodPlaylistDownloadModel::data(const QModelIndex &index, int role) const
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
VMQuickVodPlaylistDownloadModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_Rows.size();
}


void
VMQuickVodPlaylistDownloadModel::startDownloadMetaData(
        qint64 token, const QString& url, const QVariant& userData)
{
    if (url.isEmpty()) {
        qDebug() << "empty url";
        return;
    }

    if (!m_Ytdl) {
        qWarning() << "ytdl not set";
        return;
    }

    if (m_MetaDataDownloads++ == 0) {
        emit metaDataDownloadsPendingChanged();
    }

    emit busyChanged();

    m_Ytdl->startFetchMetaData(token, url, userData);
}

void
VMQuickVodPlaylistDownloadModel::startDownloadPlaylist(
        qint64 token,
        const VMPlaylist& playlist,
        QString format,
        const QString& filePath,
        const QVariant& userData) {

    qDebug() << "token" << token << "playlist" << playlist << "format" << format << "path" << filePath << "userData" << userData;

    if (!m_Ytdl) {
        qWarning() << "ytdl not set";
        return;
    }

    auto url = playlist.webPageUrl();
    auto beg = m_UrlsBeingDownloaded.cbegin();
    auto end = m_UrlsBeingDownloaded.cend();
    for (auto it = beg; it != end; ++it) {
        if (it.value() == url) {
            emit downloadFailed(url, VMVodEnums::VM_ErrorAlreadyDownloading, filePath);
            return;
        }
    }

    VMPlaylistDownloadRequest req;
    req.filePath = filePath;
    req.format = format;
    req.playlist = playlist;
    req.userData = userData;

    if (!req.isValid()) {
        qWarning() << "invalid params";
        return;
    }

    m_UrlsBeingDownloaded.insert(token, url);
    emit busyChanged();

    m_Ytdl->startFetchPlaylist(token, req);
}

void
VMQuickVodPlaylistDownloadModel::cancelDownload(int index, bool deleteFile)
{
    qDebug() << "index" << index << "delete?" << deleteFile;

    if (!m_Ytdl) {
        qWarning() << "ytdl not set";
        return;
    }

    if (index < m_Rows.size()) {
        auto handle = m_Rows[index];
        qDebug() << "handle" << handle << "delete?" << deleteFile;
        m_Ytdl->cancelFetchPlaylist(handle, deleteFile);
    }
}

void
VMQuickVodPlaylistDownloadModel::cancelDownloads(bool deleteFiles) {
    qDebug() << "delete?" << deleteFiles;

    if (!m_Ytdl) {
        qWarning() << "ytdl not set";
        return;
    }

    for (auto i = 0; i < m_Rows.size(); ++i) {
        auto handle = m_Rows[i];
        qDebug() << "cancel" << handle;
        m_Ytdl->cancelFetchPlaylist(handle, deleteFiles);
    }
}

void
VMQuickVodPlaylistDownloadModel::playlistDownloadAdded(qint64 handle, const VMPlaylistDownload& download) {
    auto it = m_Downloads.insert(handle, new VMQuickVodPlaylistDownload(this));
    it.value()->setData(download);
    beginInsertRows(QModelIndex(), m_Rows.count(), m_Rows.count());
    m_Rows << handle;
    endInsertRows();
    qDebug() << "added row" << m_Rows.count();
}

QHash<int, QByteArray>
VMQuickVodPlaylistDownloadModel::roleNames() const {
    return ms_Roles;
}

bool
VMQuickVodPlaylistDownloadModel::isUrl(QString str) const {
    QUrl url(str);
    return url.isValid();
}

bool
VMQuickVodPlaylistDownloadModel::isOnline() const {
    return m_NetworkConfigurationManager.isOnline();
}

bool
VMQuickVodPlaylistDownloadModel::isOnBroadband() const {
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
VMQuickVodPlaylistDownloadModel::isOnMobile() const {
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
VMQuickVodPlaylistDownloadModel::onOnlineChanged(bool online) {
    Q_UNUSED(online);
    emit isOnlineChanged();
    emit isOnMobileChanged();
    emit isOnBroadbandChanged();
}

bool
VMQuickVodPlaylistDownloadModel::busy() const
{
    return m_MetaDataDownloads > 0 || m_UrlsBeingDownloaded.size() > 0;
}

qint64
VMQuickVodPlaylistDownloadModel::newToken()
{
    return m_TokenGenerator++;
}

void
VMQuickVodPlaylistDownloadModel::setYtdl(VMYTDL* value)
{
    if (m_Ytdl != value) {
        if (m_Ytdl) {
            disconnect(m_Ytdl, &VMYTDL::playlistDownloadCompleted, this, &VMQuickVodPlaylistDownloadModel::onPlaylistDownloadCompleted);
            disconnect(m_Ytdl, &VMYTDL::playlistDownloadChanged, this, &VMQuickVodPlaylistDownloadModel::onPlaylistDownloadChanged);
            disconnect(m_Ytdl, &VMYTDL::metaDataDownloadCompleted, this, &VMQuickVodPlaylistDownloadModel::onMetaDataDownloadCompleted);

            if (!m_Rows.empty()) {
                beginResetModel();
                m_Rows.clear();
                endResetModel();
            }

            const auto beg = m_Downloads.begin();
            const auto end = m_Downloads.end();
            for (auto it = beg; it != end; ++it) {
                delete it.value();
            }


            m_Downloads.clear();
            m_UrlsBeingDownloaded.clear();
            m_MetaDataDownloads = 0;
            emit metaDataDownloadsPending();
            emit busyChanged();
        }

        m_Ytdl = value;

        if (m_Ytdl) {
            connect(m_Ytdl, &VMYTDL::playlistDownloadCompleted, this, &VMQuickVodPlaylistDownloadModel::onPlaylistDownloadCompleted);
            connect(m_Ytdl, &VMYTDL::playlistDownloadChanged, this, &VMQuickVodPlaylistDownloadModel::onPlaylistDownloadChanged);
            connect(m_Ytdl, &VMYTDL::metaDataDownloadCompleted, this, &VMQuickVodPlaylistDownloadModel::onMetaDataDownloadCompleted);
        }

        emit ytdlChanged();
    }
}
