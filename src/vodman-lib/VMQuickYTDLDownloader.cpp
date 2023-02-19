/* The MIT License (MIT)
 *
 * Copyright (c) 2019-2022 Jean Gressmann <jean@0x42.de>
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

#include "VMQuickYTDLDownloader.h"
#include "VMApp.h"

#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QProcess>
#include <QDebug>
#include <QUrl>

namespace
{
const QString s_ConfigFileUrl = QStringLiteral("https://www.dropbox.com/s/o8t4hypflnj6gr2/config.json?dl=1");

QNetworkRequest makeRequest(const QUrl& url)
{
    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("User-Agent", "vodman-lib " QT_STRINGIFY(VODMAN_LIB_MAJOR) "." QT_STRINGIFY(VODMAN_LIB_MINOR) "." QT_STRINGIFY(VODMAN_LIB_PATCH)); // must be set else no reply

    return request;
}

bool parseConfigFile(
        const QByteArray& buffer,
        const QString& mode,
        int* configFileVersion,
        QString* ytdlUrl,
        QString* ytdlVersion,
        QString* ytdlName)
{
    Q_ASSERT(configFileVersion);
    Q_ASSERT(ytdlUrl);
    Q_ASSERT(ytdlVersion);

    *configFileVersion = -1;

    QJsonParseError e;
    auto doc = QJsonDocument::fromJson(buffer, &e);
    if (!doc.isObject()) {
        qCritical("Malformed configuration file, expected top level {}. Parse error %s\n", qPrintable(e.errorString()));
        qDebug("%s\n", buffer.data());
        return false;
    }

    auto root = doc.object();
    *configFileVersion = root.value(QStringLiteral("version")).toInt();
    if (*configFileVersion == 0) {
        qCritical("Malformed configuration file: expected {version:int}\n");
        qDebug("%s\n", buffer.data());
        return false;
    }

    if (*configFileVersion != 1) {
        qDebug("Unsupported configuration file version %d\n", *configFileVersion);
        qDebug("%s\n", buffer.data());
        return false;
    }

    auto ytdlObject = root.value(QStringLiteral("youtube-dl")).toObject();
    if (ytdlObject.isEmpty()) {
        qCritical("Malformed configuration file: expected {youtube-dl:{}}\n");
        qDebug("%s\n", buffer.data());
        return false;
    }

    auto modeObject = ytdlObject.value(mode).toObject();
    if (modeObject.isEmpty()) {
        qCritical("Malformed configuration file: expected {youtube-dl:{%s:{}}}\n", qPrintable(mode));
        qDebug("%s\n", buffer.data());
        return false;
    }

    *ytdlUrl = modeObject.value(QStringLiteral("url")).toString();
    if (ytdlUrl->isEmpty()) {
        qCritical("Malformed configuration file: expected {youtube-dl:{%s:{url:string}}}\n", qPrintable(mode));
        qDebug("%s\n", buffer.data());
        return false;
    }

    *ytdlVersion = modeObject.value(QStringLiteral("version")).toString();
    if (ytdlVersion->isEmpty()) {
        qCritical("Malformed configuration file: expected {youtube-dl:{%s:{version:string}}}\n", qPrintable(mode));
        qDebug("%s\n", buffer.data());
        return false;
    }

    *ytdlName = modeObject.value(QStringLiteral("name")).toString();
    if (ytdlName->isEmpty()) {
        // Allow backward compatible extension to include the binary name.
        // This allows to replace youtube-dl with yt-dlp or other without
        // interrupting the user experience.
        *ytdlName = QStringLiteral("youtube-dl");
    }

    return true;
}

bool parseConfigFile(
        const QString& path,
        const QString& mode,
        int* configFileVersion,
        QString* ytdlUrl,
        QString* ytdlVersion,
        QString* ytdlName)
{
    Q_ASSERT(configFileVersion);
    Q_ASSERT(ytdlUrl);
    Q_ASSERT(ytdlVersion);

    *configFileVersion = -1;

    QFile f(path);
    if (f.open(QIODevice::ReadOnly)) {
        auto bytes = f.readAll();
        return parseConfigFile(bytes, mode, configFileVersion, ytdlUrl, ytdlVersion, ytdlName);
    } else {
        qCritical("Failed to open %s for reading: %s (%d)\n",
                  qPrintable(path), qPrintable(f.errorString()), f.error());
        return false;
    }
}

QString modeName(VMQuickYTDLDownloader::Mode value)
{
    switch (value) {
    case VMQuickYTDLDownloader::ModeTest:
        return QStringLiteral("test");
    default:
        return QStringLiteral("release");
    }
}

} // anon

/* paths:
 * CacheLocation: /home/nemo/.cache/ if statically initialized
 */
//QString VMQuickYTDLDownloader::ms_BaseDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);

VMQuickYTDLDownloader::VMQuickYTDLDownloader(QObject* parent)
    : QObject(parent)
{
    m_pythonVersion = -1;
    m_DownloadStatus = StatusUnavailable;
    m_UpdateStatus = StatusUnavailable;
    m_error = ErrorNone;
    m_dead = false;
    m_stage = None;
    m_mode = ModeRelease;
    /* /home/nemo/.cache/harbour-vodman/harbour-vodman */
    m_BaseDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);

    connect(&m_networkAccessManager, &QNetworkAccessManager::finished, this, &VMQuickYTDLDownloader::requestFinished);
    connect(&m_networkConfigurationManager, &QNetworkConfigurationManager::onlineStateChanged, this, &VMQuickYTDLDownloader::onlineStateChanged);

    if (findExecutable(QStringLiteral("python3"), QStringLiteral("/usr/bin/python3"))) {
        m_pythonVersion = 3;
    } else if (findExecutable(QStringLiteral("python"), QStringLiteral("/usr/bin/python"))) {
        m_pythonVersion = 0;
    } else {
        qCritical("Failed to find python or python3.\n");
        m_dead = true;
        m_error = ErrorPython;
    }

    if (!m_dead) {
        findBinary();
    }
}

QString VMQuickYTDLDownloader::baseDir() const
{
    return m_BaseDir + QStringLiteral("/youtube-dl");
}

QString VMQuickYTDLDownloader::modeDir(Mode value) const
{
    return baseDir() + QStringLiteral("/") + modeName(value);
}

void VMQuickYTDLDownloader::findBinary()
{
    auto mode = modeName(m_mode);
    auto baseDirectory = baseDir();
    m_ytdlPath = modeDir(m_mode) + QStringLiteral("/youtube-dl");
    emit ytdlPathChanged();
    m_ytdlVersion.clear();
    emit ytdlVersionChanged();
    m_ytdlName.clear();
    emit ytdlNameChanged();
    emit ytdlNameOrDefaultChanged();
    m_configFilePath = baseDirectory + QStringLiteral("/config.json");
    if (QDir().mkpath(baseDirectory)) {
        QFileInfo fi(m_configFilePath);
        if (fi.exists()) {
            qInfo("Configuration file %s exists.\n", qPrintable(m_configFilePath));
            fi.setFile(m_ytdlPath);
            if (fi.exists()) {
                qInfo("youtube-dl binary %s exists.\n", qPrintable(m_ytdlPath));
                int configFileVersion = 0;
                QString url;
                if (parseConfigFile(m_configFilePath, mode, &configFileVersion, &url, &m_ytdlVersion, &m_ytdlName)) {
                    qInfo("%s version %s, mode %s\n", qPrintable(m_ytdlName), qPrintable(m_ytdlVersion), qPrintable(mode));
                    setError(ErrorNone);
                    setDownloadStatus(StatusReady);
                    emit ytdlVersionChanged();
                    emit ytdlNameChanged();
                    emit ytdlNameOrDefaultChanged();
                } else {
                    setDownloadStatus(StatusUnavailable);
                    if (configFileVersion == -1) {
                        setError(ErrorUnsupportedConfigurationFileFormat);
                    } else {
                        QFile::remove(m_configFilePath);
                        setError(ErrorUnsupportedConfigurationFileFormat);
                    }
                }
            } else {
                setError(ErrorNone);
                setDownloadStatus(StatusUnavailable);
                qInfo("youtube-dl binary %s doesn't exist.\n", qPrintable(m_ytdlPath));
            }
        } else {
            setError(ErrorNone);
            setDownloadStatus(StatusUnavailable);
            qInfo("Configuration file %s doesn't exist.\n", qPrintable(m_configFilePath));
        }
    } else {
        qCritical("Failed to create directory %s.\n", qPrintable(baseDirectory));
        setDownloadStatus(StatusUnavailable);
        setError(ErrorFileOrDirCreationFailed);
    }
}

void VMQuickYTDLDownloader::setMode(Mode value)
{
    if (m_dead) {
        qWarning("dead, no change to mode\n");
        return;
    }

    if (busy()) {
        setError(ErrorBusy);
        return;
    }

    if (value != m_mode) {
        m_mode = value;
        emit modeChanged();
        qDebug("mode changed to %s\n", qPrintable(modeName(m_mode)));
        findBinary();
        m_UpdateStatus = StatusUnavailable;
        m_UpdateVersion.clear();
        emit isUpdateAvailableChanged();
        emit updateVersionChanged();
    }
}

void VMQuickYTDLDownloader::setDownloadStatus(Status value)
{
    if (value != m_DownloadStatus) {
        m_DownloadStatus = value;
        emit downloadStatusChanged();
        emit busyChanged();
    }
}

void VMQuickYTDLDownloader::setUpdateStatus(Status value)
{
    if (value != m_UpdateStatus) {
        m_UpdateStatus = value;
        emit updateStatusChanged();
        emit busyChanged();
    }
}

void VMQuickYTDLDownloader::setError(Error value)
{
    if (value != m_error) {
        m_error = value;
        emit errorChanged();
    }
}

bool
VMQuickYTDLDownloader::findExecutable(const QString& name, const QString& path)
{
     QFileInfo fi(path);
     if (fi.exists() && fi.isFile()) {
         qInfo("File %s exists\n", qPrintable(fi.filePath()));
         return true;
     }

     qInfo("File %s doesn't exist.\n", qPrintable(fi.filePath()));

     QStringList arguments;
     arguments << QStringLiteral("-c") << QStringLiteral("which %1").arg(name);

     QProcess process;
     process.start(QStringLiteral("/bin/sh"), arguments, QIODevice::ReadOnly);
     process.waitForFinished();
     if (process.exitStatus() == QProcess::NormalExit &&
         process.exitCode() == 0) {
         auto result = process.readAllStandardOutput();
         result = result.simplified();
         if (result.isEmpty()) {
            qInfo("Could not find '%s' using 'which'\n", qPrintable(name));
         } else {
            qInfo("Found '%s' as %s\n", qPrintable(name), qPrintable(result));
         }

         return !result.isEmpty();
     } else {
         qDebug(
             "Failed to start /bin/sh -c \"which %s\" exitStatus=%d, exitCode=%d)\nstdout: %s\nstderr:%s\n",
             qPrintable(name),
             static_cast<int>(process.exitStatus()), static_cast<int>(process.exitCode()),
             qPrintable(process.readAllStandardOutput()), qPrintable(process.readAllStandardError()));
     }


     return false;
}

void
VMQuickYTDLDownloader::download()
{
    if (m_dead) {
        qWarning("dead, not downloading\n");
        return;
    }

    if (busy()) {
        setError(ErrorBusy);
        return;
    }

    if (!m_networkConfigurationManager.isOnline()) {
        setError(ErrorOffline);
        return;
    }

    m_stage = ConfigFile;
    m_networkAccessManager.get(makeRequest(s_ConfigFileUrl));
    setError(ErrorNone);
    setDownloadStatus(StatusDownloading);
}

void
VMQuickYTDLDownloader::checkForUpdate()
{
    if (m_dead) {
        qWarning("dead, not checking for update\n");
        return;
    }

    if (busy()) {
        setError(ErrorBusy);
        return;
    }

    if (!m_networkConfigurationManager.isOnline()) {
        setError(ErrorOffline);
        return;
    }

    m_UpdateVersion.clear();
    emit updateVersionChanged();
    emit isUpdateAvailableChanged();

    m_stage = ConfigFile;
    m_networkAccessManager.get(makeRequest(s_ConfigFileUrl));
    setError(ErrorNone);
    setUpdateStatus(StatusDownloading);
}

void
VMQuickYTDLDownloader::requestFinished(QNetworkReply *reply)
{
    reply->deleteLater();

    switch (reply->error()) {
    case QNetworkReply::OperationCanceledError:
        if (StatusDownloading == m_DownloadStatus) {
            setDownloadStatus(StatusUnavailable);
        } else {
            setUpdateStatus(StatusUnavailable);
        }
        break;
    case QNetworkReply::NoError: {
        // Get the http status code
        int v = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (v >= 200 && v < 300) { // Success
            // Here we got the final reply
            auto bytes = reply->readAll();
            switch (m_stage) {
            case ConfigFile: {
                if (StatusDownloading == m_DownloadStatus) {
                    QFile f(m_configFilePath);
                    if (f.open(QIODevice::Truncate | QIODevice::WriteOnly)) {
                        auto w = f.write(bytes);
                        if (w == bytes.size() && f.flush()) {
                            qInfo("Stored configuration file at %s\n", qPrintable(m_configFilePath));
                            int configFileVersion = 0;
                            QString url;
                            auto mode = modeName(m_mode);
                            if (parseConfigFile(m_configFilePath, mode, &configFileVersion, &url, &m_ytdlVersion, &m_ytdlName)) {
                                qInfo("%s version %s, mode %s\n", qPrintable(m_ytdlName), qPrintable(m_ytdlVersion), qPrintable(mode));
                                emit ytdlVersionChanged();
                                emit ytdlNameChanged();
                                emit ytdlNameOrDefaultChanged();
                                m_UpdateVersion = m_ytdlVersion;
                                emit updateVersionChanged();
                                emit isUpdateAvailableChanged();
                                setUpdateStatus(StatusReady);
                                m_stage = Binary;
                                m_networkAccessManager.get(makeRequest(url));
                            } else if (-1 == configFileVersion) {
                                setDownloadStatus(StatusUnavailable);
                                setError(ErrorUnsupportedConfigurationFileFormat);
                            } else {
                                QFile::remove(m_configFilePath);
                                setDownloadStatus(StatusUnavailable);
                                setError(ErrorMalformedConfigurationFile);
                            }
                        } else {
                            setDownloadStatus(StatusUnavailable);
                            setError(ErrorNoSpaceLeftOnDevice);
                            qCritical("Failed to save configuration file %s: %s (%d)\n",
                                      qPrintable(m_configFilePath), qPrintable(f.errorString()), f.error());
                        }
                    } else {
                        setDownloadStatus(StatusUnavailable);
                        setError(ErrorFileOrDirCreationFailed);
                        qCritical("Failed to create/truncate configuration file %s: %s (%d)\n",
                                  qPrintable(m_configFilePath), qPrintable(f.errorString()), f.error());
                    }
                } else {
                    auto mode = modeName(m_mode);
                    int configFileVersion = 0;
                    QString url;
                    if (parseConfigFile(bytes, mode, &configFileVersion, &url, &m_UpdateVersion, &m_UpdateName)) {
                        qInfo("mode %s %s version %s available\n", qPrintable(mode), qPrintable(m_UpdateName), qPrintable(m_UpdateVersion));
                        emit updateVersionChanged();
                        emit updateNameChanged();
                        emit isUpdateAvailableChanged();
                        setUpdateStatus(StatusReady);
                    } else if (-1 == configFileVersion) {
                        setUpdateStatus(StatusUnavailable);
                        setError(ErrorUnsupportedConfigurationFileFormat);
                    }
                }
            } break;
            case Binary: {
                // First line should be either
                // #!/usr/bin/env python
                // or
                // #!/usr/bin/env python3
                const auto END_OF_PYTHON = 0x15;
                quint8 byte = bytes[END_OF_PYTHON];
                auto is_python = byte == ' ' || byte == '\r' || byte == '\n';
                qDebug("byte at binary off %u %02x (%c)\n", END_OF_PYTHON, byte, byte);

                if (3 == m_pythonVersion && is_python) { // update binary to start python3
                    bytes.insert(END_OF_PYTHON, '3');
                }

                if (QDir().mkpath(modeDir(m_mode))) {
                    QFile f(m_ytdlPath);
                    if (f.open(QIODevice::Truncate | QIODevice::WriteOnly)) {
                        auto w = f.write(bytes);
                        if (w == bytes.size() && f.flush()) {
                            qInfo("Stored %s at %s\n", qPrintable(m_UpdateName), qPrintable(m_ytdlPath));
                            if (f.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner | QFile::ReadGroup | QFile::ExeGroup | QFile::ReadOther | QFile::ExeOther)) {
                                qInfo("Changed permissions of %s to 0755\n", qPrintable(m_ytdlPath));
                                setDownloadStatus(StatusReady);
                                setError(ErrorNone);
                            } else {
                                qCritical("Failed to change permissions of %s to 0755: %s (%d)\n", qPrintable(m_ytdlPath), qPrintable(f.errorString()), static_cast<int>(f.error()));
                                setDownloadStatus(StatusUnavailable);
                                setError(ErrorFileOrDirCreationFailed);
                            }
                        } else {
                            setDownloadStatus(StatusUnavailable);
                            setError(ErrorNoSpaceLeftOnDevice);
                            qCritical("Failed to save %s to %s: %s (%d)\n",
                                      qPrintable(m_UpdateName), qPrintable(m_ytdlPath), qPrintable(f.errorString()), f.error());
                        }
                    } else {
                        setDownloadStatus(StatusUnavailable);
                        setError(ErrorFileOrDirCreationFailed);
                        qCritical("Failed to create/truncate %s file %s: %s (%d)\n",
                                  qPrintable(m_UpdateName), qPrintable(m_ytdlPath), qPrintable(f.errorString()), f.error());
                    }
                } else {
                    setDownloadStatus(StatusUnavailable);
                    setError(ErrorFileOrDirCreationFailed);
                    qCritical("Failed to create directory %s\n", qPrintable(modeDir(m_mode)));
                }
            } break;
            default:
                setDownloadStatus(StatusUnavailable);
                setError(ErrorUnknown);
                qCritical("Unknown download stage %d\n", m_stage);
                break;
            }
        } else if (v >= 300 && v < 400) { // Redirection
            // Get the redirection url
            auto newUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
            // Because the redirection url can be relative,
            // we have to use the previous one to resolve it
            newUrl = reply->url().resolved(newUrl);

            m_networkAccessManager.get(makeRequest(newUrl));
        } else  {
            qDebug() << "Http status code:" << v;
            if (StatusDownloading == m_DownloadStatus) {
                setDownloadStatus(StatusUnavailable);
            } else {
                setUpdateStatus(StatusUnavailable);
            }
            setError(ErrorDownloadFailed);
        }
    } break;
    default: {
        qDebug() << "Network request failed: " << reply->error() << reply->errorString() << reply->url();
        if (StatusDownloading == m_DownloadStatus) {
            setDownloadStatus(StatusUnavailable);
        } else {
            setUpdateStatus(StatusUnavailable);
        }
        setError(ErrorDownloadFailed);
    } break;
    }
}

void
VMQuickYTDLDownloader::remove()
{
    if (m_dead) {
        qWarning("dead, nothing removed\n");
        return;
    }

    if (QFile::remove(m_configFilePath)) {
        qInfo("Removed %s\n", qPrintable(m_configFilePath));
    }

    if (QFile::remove(m_ytdlPath)) {
        qInfo("Removed %s\n", qPrintable(m_ytdlPath));
    }

    setDownloadStatus(StatusUnavailable);
    setError(ErrorNone);
}

bool
VMQuickYTDLDownloader::busy() const
{
    return StatusDownloading == m_DownloadStatus || StatusDownloading == m_UpdateStatus;
}

void
VMQuickYTDLDownloader::onlineStateChanged(bool)
{
    emit isOnlineChanged();
}

bool
VMQuickYTDLDownloader::isUpdateAvailable() const
{
    return !m_UpdateVersion.isEmpty() &&
            !m_ytdlVersion.isEmpty() &&
            m_UpdateVersion.compare(m_ytdlVersion) > 0;
}

QString
VMQuickYTDLDownloader::ytdlNameOrDefault() const
{
    if (m_ytdlName.isEmpty()) {
        return ytdlDefaultName();
    }

    return m_ytdlName;
}
