/* The MIT License (MIT)
 *
 * Copyright (c) 2018-2022 Jean Gressmann <jean@0x42.de>
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

#include "VMYTDL.h"
#include "VMPlaylist.h"
#include "VMDownload.h"
#include "VMApp.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>
#include <QProcess>
#include <QDebug>
#include <QRegExp>
#include <QUrlQuery>
#include <QFile>
#include <QFileInfo>

using IndexList = QVector<int>;
Q_DECLARE_METATYPE(IndexList)

namespace {
//[download] Destination: youtube-dl tutorial for windows-fKe9rV-gl1c.f251.webm
//[download] METAL ALLEGIANCE - Anaheim Show 2019 (OFFICIAL TRAILER)-dmhgRlaReho.mp4 has already been downloaded

const QRegExp s_YTDLVideoNumberRegexp("^\\[download\\]\\s+Downloading video (\\d+) of \\d+\\s*");
const QRegExp s_YTDLDestinationRegexp("^\\[download\\]\\s+Destination:\\s+(.+)\\s*$");
const QRegExp s_YTDLAlreadyDownloadedRegexp("^\\[download\\]\\s+(.+)\\s+has already been downloaded\\s*$");
const QRegExp s_YTDLProgressRegexp("^\\[download\\]\\s+(\\d+(?:\\.\\d+)?)%\\s+");
const QString s_Token = QLatin1String("token");
const QString s_Type = QLatin1String("type");
const QString s_MetaData = QLatin1String("metadata");
const QString s_Download = QLatin1String("download");
const QString s_Indices = QLatin1String("indices");
const QString s_Duration = QLatin1String("duration");
const QString s_NoCallHome = QLatin1String("--no-call-home");
const QString s_NoColor = QLatin1String("--no-color");



void Nop(QString&) {}

void ParseYtdlOutput(QString& str, VMPlaylistDownloadData& downloadData, const QVector<int>& indices, int downloadDuration)
{
    qDebug("%s\n", qPrintable(str));
    if (s_YTDLProgressRegexp.indexIn(str) != -1) {
        QString capture = s_YTDLProgressRegexp.cap(1);
        bool ok = false;
        auto value = capture.toFloat(&ok);
        if (ok) {
//            qDebug() << str;
            auto normalized = qMax(0.0f, qMin(value*1e-2f, 1.0f));
            downloadData.files[downloadData.currentFileIndex].data().progress = normalized;
            if (downloadDuration > 0) {
                // compute total progress
                float totalProgress = 0;
                for (int i = 0; i < downloadData.currentFileIndex; ++i) {
                    totalProgress += downloadData.playlist._vods()[i].duration();
                }
                totalProgress += normalized * downloadData.playlist._vods()[downloadData.currentFileIndex].duration();
                totalProgress /= downloadDuration;
                downloadData.progress = totalProgress;
            } else {
                downloadData.progress = normalized;
            }
        }
    } else if (s_YTDLVideoNumberRegexp.indexIn(str) != -1) {
        QString capture = s_YTDLVideoNumberRegexp.cap(1);
        bool ok = false;
        auto value = capture.toInt(&ok);
        if (ok && value >= 1) {
            auto fileIndex = value - 1;
            if (fileIndex != downloadData.currentFileIndex) {
                auto& vodFileData = downloadData.files[downloadData.currentFileIndex].data();
                downloadData.fileSize -= vodFileData.fileSize;
                QFileInfo fi(vodFileData.filePath);
                if (fi.exists()) {
                    vodFileData.fileSize = fi.size();
                } else {
                    vodFileData.fileSize = 0;
                }
                downloadData.fileSize += vodFileData.fileSize;

                downloadData.currentFileIndex = qMin(fileIndex, downloadData.playlist.vods() - 1);
                VMFileDownload fileDownload;
                fileDownload.data().playlistIndex = indices[downloadData.currentFileIndex];
                downloadData.files.append(fileDownload);
            }
        }
    } else if (s_YTDLDestinationRegexp.indexIn(str) != -1) {
        downloadData.files[downloadData.currentFileIndex].data().filePath = s_YTDLDestinationRegexp.cap(1);
    } else if (s_YTDLAlreadyDownloadedRegexp.indexIn(str) != -1) {
        downloadData.files[downloadData.currentFileIndex].data().filePath = s_YTDLAlreadyDownloadedRegexp.cap(1);
    }
}

} // anon

VMYTDL::~VMYTDL()
{
    disconnect();
    foreach (QProcess* p, m_ProcessMap.keys()) {
        p->kill();
        delete p;
    }
}

VMYTDL::VMYTDL(QObject* parent)
    : QObject(parent)
    , m_Normalizer(&Nop)
    , m_MetaDataSecondsValid(3600) // one hour timeout, typically vod file urls go stale
    , m_YtdlVerbose(false)
{
}

QString
VMYTDL::ytdlPath() const
{
    return m_YoutubeDl_Path;
}

void
VMYTDL::setYtdlPath(const QString& path)
{
    if (m_YoutubeDl_Path != path) {
        m_YoutubeDl_Path = path;
        emit ytdlPathChanged();
    }
}

void
VMYTDL::setMetaDataCacheCapacity(int value)
{
    if (value < 0) {
        value = 100;
    }

    if (m_MetaDataCache.maxCost() != value) {
        m_MetaDataCache.setMaxCost(value);
        emit metaDataCacheCapacityChanged();
    }
}

void
VMYTDL::setMetaDataSecondsValid(int value)
{
    if (m_MetaDataSecondsValid != value) {
        m_MetaDataSecondsValid= value;
        emit metaDataSecondsValidChanged();
    }
}

bool
VMYTDL::available() const
{
    return !m_YoutubeDl_Path.isEmpty();
}

void
VMYTDL::startFetchMetaData(qint64 token, const QString& _url, const QVariant& userData) {

    VMMetaDataDownload download;
    VMMetaDataDownloadData& data = download.data();

    QString url(_url);
    m_Normalizer(url);

    data.userData = userData;
    data.url = url;
    if (!available()) {
        data.errorMessage = QStringLiteral("path to " VM_GEN_YOUTUBE_DL_NAME " no set");
        data.error = VMVodEnums::VM_ErrorNoYoutubeDl;
        qDebug() << data.errorMessage;
        emit metaDataDownloadCompleted(token, download);
        return;
    }

    qDebug() << "Fetching video meta data for" << url;

    auto cacheEntryPtr = m_MetaDataCache[download.data().url];
    if (cacheEntryPtr) {
        auto now = QDateTime::currentDateTime();
        if (m_MetaDataSecondsValid < 0 || now <= cacheEntryPtr->fetchTime.addSecs(m_MetaDataSecondsValid)) {
            qDebug() << "Response for" << download.data().url << "available in cache, using it";
            data.playlist = cacheEntryPtr->playlist;
            data.error = VMVodEnums::VM_ErrorNone;
            emit metaDataDownloadCompleted(token, download);
            return;
        }

        m_MetaDataCache.remove(download.data().url);
    }

    QVariantMap result;
    result[s_Type] = s_MetaData;
    result[s_Download] = QVariant::fromValue(download);
    result[s_Token] = token;


    QStringList arguments;
    if (m_YtdlVerbose) {
        arguments << QStringLiteral("--verbose");
    }

    if (m_CacheDirectory.isEmpty()) {
        arguments << QStringLiteral("--no-cache-dir");
    } else {
        arguments << QStringLiteral("--cache-dir") << m_CacheDirectory;
    }

    arguments << QStringLiteral("--dump-json")
              << QStringLiteral("--youtube-skip-dash-manifest")
              << s_NoCallHome
              << s_NoColor
              << m_CustomOptions
              << url;

    qDebug() << VM_GEN_YOUTUBE_DL_NAME " subprocess:" << m_YoutubeDl_Path << arguments;

    auto process = createProcess();
    connect(process, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(onMetaDataProcessFinished(int, QProcess::ExitStatus)));
    connect(process, SIGNAL(error(QProcess::ProcessError)),
            this, SLOT(onProcessError(QProcess::ProcessError)));

    m_ProcessMap.insert(process, result);

    process->start(m_YoutubeDl_Path, arguments, QIODevice::ReadOnly);
}


void
VMYTDL::startFetchPlaylist(qint64 token, const VMPlaylistDownloadRequest& req, VMPlaylistDownload* _download)
{
    VMPlaylistDownload download;

    if (!_download) {
        _download = &download;
    }

    VMPlaylistDownloadData& data = _download->data();
    data.playlist = req.playlist;
    data.format = req.format;
    data.userData = req.userData;
    data.timeStarted = data.timeChanged = QDateTime::currentDateTime();
    data.files.append(VMFileDownload());

    if (!req.isValid()) {
        data.errorMessage = QStringLiteral("invalid download request");
        data.error = VMVodEnums::VM_ErrorInvalidRequest;
        qDebug() << data.errorMessage;
        emit playlistDownloadCompleted(token, *_download);
        return;
    }

    if (!available()) {
        data.errorMessage = QStringLiteral("path to " VM_GEN_YOUTUBE_DL_NAME " no set");
        data.error = VMVodEnums::VM_ErrorNoYoutubeDl;
        qDebug() << data.errorMessage;
        emit playlistDownloadCompleted(token, *_download);
        return;
    }

    QVariantMap result;
    result[s_Type] = QStringLiteral("playlist");
    result[s_Download] = QVariant::fromValue(*_download);
    result[s_Token] = token;


    qDebug() << "Trying to obtain" << (req.indices.isEmpty() ? data.playlist.vods() : req.indices.size())
             << "video file(s) for:" << data.playlist.webPageUrl() << "format:"
             << req.format << "file path:" << req.filePath;

    QStringList arguments;
    if (m_YtdlVerbose) {
        arguments << QStringLiteral("--verbose");
    }

    if (m_CacheDirectory.isEmpty()) {
        arguments << QStringLiteral("--no-cache-dir");
    } else {
        arguments << QStringLiteral("--cache-dir") << m_CacheDirectory;
    }

    arguments //<< QStringLiteral("-c")
              << QStringLiteral("--newline")
              << QStringLiteral("--no-part")
              << QStringLiteral("--fixup") << QStringLiteral("never")
              << s_NoCallHome
              << s_NoColor
              << QStringLiteral("--hls-prefer-native") // ffmpeg seems to be on device (SFOS 4.2.0) but was built w/o TLS
              << QStringLiteral("-f") << req.format
              << QStringLiteral("-o") << req.filePath
              << m_CustomOptions;

    if (req.indices.isEmpty()) {
        QVector<int> indices;
        indices.resize(req.playlist.vods());
        for (int i = 0; i < indices.size(); ++i) {
            indices[i] = req.playlist._vods()[i].playlistIndex();
        }
        result[s_Indices] = QVariant::fromValue(indices);
        result[s_Duration] = req.playlist.duration();
        data.files[0].data().playlistIndex = indices[0];
    } else {
        int duration = 0;
        for (auto index : req.indices) {
            for (const auto& file : req.playlist._vods()) {
                if (index == file.playlistIndex()) {
                    duration += file.duration();
                    break;
                }
            }
        }

        result[s_Indices] = QVariant::fromValue(req.indices);
        result[s_Duration] = duration;
        data.files[0].data().playlistIndex = req.indices[0];

        QString items;
        for (auto index : req.indices) {
            if (!items.isEmpty()) {
                items += QStringLiteral(",");
            }
            items += QString::number(index);
        }

        arguments << QStringLiteral("--playlist-items") << items;
    }

    arguments << data.playlist.webPageUrl();

    qDebug() << VM_GEN_YOUTUBE_DL_NAME " subprocess:" << m_YoutubeDl_Path << arguments;

    QProcess* process = createProcess();
    connect(process, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(onPlaylistProcessFinished(int,QProcess::ExitStatus)));
    connect(process, &QProcess::readyReadStandardOutput,
            this, &VMYTDL::onYoutubeDlVodFileDownloadProcessReadReady);
    connect(process, SIGNAL(errorOccurred(QProcess::ProcessError)),
            this, SLOT(onProcessError(QProcess::ProcessError)));


    m_ProcessMap.insert(process, result);
    m_VodDownloads.insert(token, process);

    process->start(m_YoutubeDl_Path, arguments, QIODevice::ReadOnly);
}

void
VMYTDL::cancelFetchPlaylist(qint64 token, bool deleteFile) {
    qDebug() << "token" << token << "delete?" << deleteFile;

    QProcess* process = m_VodDownloads.value(token);
    if (process) {
        const auto& m = m_ProcessMap[process];
        auto download = qvariant_cast<VMPlaylistDownload>(m[s_Download]);
        auto& data = download.data();
        data.error = VMVodEnums::VM_ErrorCanceled;

        qDebug() << "kill pid" << process->processId();
        process->kill();

        if (deleteFile) {
            for (auto i = 0; i < data.files.size(); ++i) {
                auto filePath = data.files[i].filePath();
                if (!filePath.isEmpty()) {
                    qDebug() << "removing" << filePath;
                    QFile::remove(filePath);
                }
            }
        }
    }
}

void
VMYTDL::onMetaDataProcessFinished(int code, QProcess::ExitStatus status)
{
    QProcess* process = qobject_cast<QProcess*>(QObject::sender());
    Q_ASSERT(process);

    cleanupProcess(process);

    qDebug() << VM_GEN_YOUTUBE_DL_NAME " meta data process pid" << process->processId()
             << "finished, status:" << status
             << ", exit code:" << code;

    auto result = m_ProcessMap.value(process);
    m_ProcessMap.remove(process);

    const qint64 id = qvariant_cast<qint64>(result[s_Token]);
    VMMetaDataDownload download = qvariant_cast<VMMetaDataDownload>(result[s_Download]);
    VMMetaDataDownloadData& downLoadData = download.data();
    downLoadData.error = VMVodEnums::VM_ErrorNone;

    QString processError = QString::fromUtf8(process->readAllStandardError());
    auto errorLines = processError.splitRef(QChar('\n'), QString::SkipEmptyParts);
    for (int i = 0; i < errorLines.size(); ++i) {
        const auto& line = errorLines[i];
        qDebug() << "stderr" << line;
        if (line.indexOf(QStringLiteral("ERROR:")) == 0) {
            downLoadData.errorMessage = line.toString();
            if (line.indexOf(QStringLiteral("unsupported url"), 0, Qt::CaseInsensitive) >= 0) {
                downLoadData.error = VMVodEnums::VM_ErrorUnsupportedUrl;
            } else if (line.indexOf(QStringLiteral("requested format not available"), 0, Qt::CaseInsensitive) >= 0) {
                downLoadData.error = VMVodEnums::VM_ErrorFormatNotAvailable;
            } else if (line.indexOf(QStringLiteral("[Errno -2]"), 0, Qt::CaseInsensitive) >= 0) {
                //ERROR: Unable to download webpage: <urlopen error [Errno -2] Name or service not known> (caused by URLError(gaierror(-2, 'Name or service not known'),))
                downLoadData.error = VMVodEnums::VM_ErrorNetworkDown;
            } else if (line.indexOf(QStringLiteral("[Errno -3]"), 0, Qt::CaseInsensitive) >= 0) {
                // "ERROR: Failed to download m3u8 information: <urlopen error [Errno -3] Temporary failure in name resolution> (caused by URLError(gaierror(-3, 'Temporary failure in name resolution')))"
                downLoadData.error = VMVodEnums::VM_ErrorTemporaryFailureNameResolution;
            } else if (line.indexOf(QStringLiteral("is not a valid URL"), 0, Qt::CaseInsensitive) >= 0) {
                //ERROR: '' is not a valid URL. Set --default-search \"ytsearch\" (or run  youtube-dl \"ytsearch:\" ) to search YouTube
                downLoadData.error = VMVodEnums::VM_ErrorInvalidUrl;
            } else if (line.indexOf(QStringLiteral("This video is unavailable."), 0, Qt::CaseInsensitive) >= 0) {
                downLoadData.error = VMVodEnums::VM_ErrorContentGone;
            } else if (line.indexOf(QStringLiteral("Time-out"), 0, Qt::CaseInsensitive) >= 0) {
                // ERROR: Failed to download m3u8 information: HTTP Error 504: Gateway Time-out (caused by HTTPError());
                downLoadData.error = VMVodEnums::VM_ErrorTimedOut;
            } else if (line.indexOf(QStringLiteral("HTTP Error 404"), 0, Qt::CaseInsensitive) >= 0) {
                // ERROR: Unable to download webpage: HTTP Error 404: Not Found (caused by <HTTPError 404: 'Not Found'>);
                downLoadData.error = VMVodEnums::VM_ErrorInvalidUrl;
            } else if (line.indexOf(QStringLiteral("Signature extraction failed"), 0, Qt::CaseInsensitive) >= 0) {
                // ERROR: Signature extraction failed
                downLoadData.error = VMVodEnums::VM_ErrorSignatureExtractionFailed;
            } else if (line.indexOf(QStringLiteral("Forbidden"), 0, Qt::CaseInsensitive) >= 0) {
                // ERROR: unable to download video data: HTTP Error 403: Forbidden"
                downLoadData.error = VMVodEnums::VM_ErrorAccess;
            } else if (line.indexOf(QStringLiteral("Too Many Requests"), 0, Qt::CaseInsensitive) >= 0) {
                // ERROR: Unable to download webpage: HTTP Error 429: Too Many Requests (caused by <HTTPError 429: 'Too Many Requests'>); please report this issue on https://yt-dl.org/bug . Make sure you are using the latest version; type  youtube-dl -U  to update. Be sure to call youtube-dl with the --verbose flag and include its complete output."
                downLoadData.error = VMVodEnums::VM_ErrorTooManyRequests;
            } else {
                downLoadData.error = VMVodEnums::VM_ErrorUnknown;
            }
            emit metaDataDownloadCompleted(id, download);
            return;
        }
    }

    QByteArray output = process->readAllStandardOutput();
    QVector<int> starts, ends;

    if (!parseJson(output, &starts, &ends)) {
        downLoadData.error = VMVodEnums::VM_ErrorInvalidResponse;
        downLoadData.errorMessage = QStringLiteral("Failed to determine location of JSON objects in output");
        qDebug("%s\n", qPrintable(output));
        emit metaDataDownloadCompleted(id, download);
        return;
    }

    qDebug("JSON object 0 at [%d-%d)\n", starts[0], ends[0]);
    QByteArray first = output.mid(starts[0], ends[0] - starts[0]);
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(first, &error);
    if (error.error != QJsonParseError::NoError) {
        downLoadData.error = VMVodEnums::VM_ErrorInvalidResponse;
        downLoadData.errorMessage = QStringLiteral("JSON parse error: %1").arg(error.errorString());
        qCritical() << "JSON parse error:" << error.errorString();
        emit metaDataDownloadCompleted(id, download);
        return;
    }

    // see json-responses.txt for examples

    VMPlaylistData& vodPlaylistData = downLoadData.playlist.data();
    QJsonObject root = doc.object();

    // This will have each individual video URL for youtube
    // it works for afreecatv though
//    descData.webPageUrl = root.value(QStringLiteral("webpage_url")).toString();
//    if (descData.webPageUrl.isEmpty()) {
//        QString message = QStringLiteral("Invalid youtube-dl JSON response: ") + output.data();
//        qCritical() << message;
//        downLoadData.error = VMVodEnums::VM_ErrorInvalidResponse;
//        downLoadData.errorMessage = message;
//        emit metaDataDownloadCompleted(id, download);
//        return;
//    }

    // we need to forward the actual playlist url, not the url of the first
    // video which is what we get with youtube.com
    vodPlaylistData.webPageUrl = download.url();


    auto playlistId = root.value(QStringLiteral("playlist_id")).toString();
    if (playlistId.isEmpty()) {
        // simple vod, forward vod data
        vodPlaylistData.title = root.value(QStringLiteral("title")).toString();
        vodPlaylistData.id = root.value(QStringLiteral("id")).toString();
    } else {
        // playlist
        vodPlaylistData.title = root.value(QStringLiteral("playlist_title")).toString();
        vodPlaylistData.id = root.value(QStringLiteral("playlist_id")).toString();
    }

    vodPlaylistData.vods.reserve(ends.size());

    if (!appendVod(vodPlaylistData, root)) {
        QString message = QStringLiteral("No format has video");
        qCritical() << message;
        downLoadData.error = VMVodEnums::VM_ErrorNoVideo;
        downLoadData.errorMessage = message;
        emit metaDataDownloadCompleted(id, download);
        return;
    }

    for (int i = 1; i < ends.size(); ++i) {
        qDebug("JSON object %d at [%d-%d)\n", i, starts[i], ends[i]);
        auto bytes = output.mid(starts[i], ends[i] - starts[i]);
        doc = QJsonDocument::fromJson(bytes, &error);
        if (error.error != QJsonParseError::NoError) {
            downLoadData.error = VMVodEnums::VM_ErrorInvalidResponse;
            downLoadData.errorMessage = QStringLiteral("JSON parse error: %1").arg(error.errorString());
            qCritical() << "JSON parse error:" << error.errorString();
            qDebug("%s\n", bytes.data());
            emit metaDataDownloadCompleted(id, download);
            return;
        }

        root = doc.object();
        if (!appendVod(vodPlaylistData, root)) {
            QString message = QStringLiteral("No format has video");
            qCritical() << message;
            downLoadData.error = VMVodEnums::VM_ErrorNoVideo;
            downLoadData.errorMessage = message;
            emit metaDataDownloadCompleted(id, download);
            return;
        }
    }

    auto cacheEntry = new CacheEntry;
    cacheEntry->fetchTime = QDateTime::currentDateTime();
    cacheEntry->playlist = downLoadData.playlist;
    m_MetaDataCache.insert(downLoadData.url, cacheEntry);

    qDebug() << "emit fetchVodMetaDataCompleted("<< id << ", " << download << ")";
    emit metaDataDownloadCompleted(id, download);
}

void
VMYTDL::appendVideoFormat(VMVodData& data, const QJsonObject& format) const
{
    data.videoFormats.append(VMVideoFormat());
    VMVideoFormat& videoFormat = data.videoFormats.last();
    VMVideoFormatData& videoFormatData = videoFormat.data();

    videoFormatData.width = format.value(QStringLiteral("width")).toInt();
    videoFormatData.height = format.value(QStringLiteral("height")).toInt();
    videoFormatData.id = format.value(QStringLiteral("format_id")).toString();
    videoFormatData.streamUrl = format.value(QStringLiteral("url")).toString();
    videoFormatData.manifestUrl = format.value(QStringLiteral("manifest_url")).toString();
    videoFormatData.displayName = format.value(QStringLiteral("format")).toString();
    videoFormatData.extension = format.value(QStringLiteral("ext")).toString();
    videoFormatData.tbr = static_cast<float>(format.value(QStringLiteral("tbr")).toDouble());
    videoFormatData.codec = format.value(QStringLiteral("vcodec")).toString();
    fillFormatId(videoFormatData);
    fillWidth(videoFormatData);

    qDebug() << "added video format" << videoFormat;
}

void
VMYTDL::appendAudioFormat(VMVodData& data, const QJsonObject& format) const
{
    data.audioFormats.append(VMAudioFormat());
    VMAudioFormat& audioFormat = data.audioFormats.last();
    VMAudioFormatData& audioFormatData = audioFormat.data();

    audioFormatData.id = format.value(QStringLiteral("format_id")).toString();
    audioFormatData.streamUrl = format.value(QStringLiteral("url")).toString();
    audioFormatData.manifestUrl = format.value(QStringLiteral("manifest_url")).toString();
    audioFormatData.displayName = format.value(QStringLiteral("format")).toString();
    audioFormatData.extension = format.value(QStringLiteral("ext")).toString();
    audioFormatData.abr = static_cast<float>(format.value(QStringLiteral("abr")).toDouble());
    audioFormatData.codec = format.value(QStringLiteral("acodec")).toString();

    qDebug() << "added audio format" << audioFormat;
}


void
VMYTDL::appendAvFormat(VMVodData& data, const QJsonObject& format) const
{
    data.avFormats.append(VMVideoFormat());
    VMVideoFormat& avFormat = data.avFormats.last();
    VMVideoFormatData& avFormatData = avFormat.data();

    avFormatData.width = format.value(QStringLiteral("width")).toInt();
    avFormatData.height = format.value(QStringLiteral("height")).toInt();
    avFormatData.id = format.value(QStringLiteral("format_id")).toString();
    avFormatData.streamUrl = format.value(QStringLiteral("url")).toString();
    avFormatData.manifestUrl = format.value(QStringLiteral("manifest_url")).toString();
    avFormatData.displayName = format.value(QStringLiteral("format")).toString();
    avFormatData.extension = format.value(QStringLiteral("ext")).toString();
    avFormatData.tbr = static_cast<float>(format.value(QStringLiteral("tbr")).toDouble());
    avFormatData.codec = format.value(QStringLiteral("vcodec")).toString() + QStringLiteral("+") + format.value(QStringLiteral("acodec")).toString();
    fillFormatId(avFormatData);
    fillWidth(avFormatData);

    qDebug() << "added av format" << avFormat;
}

bool
VMYTDL::appendVod(VMPlaylistData& vodPlaylistData, const QJsonObject& root) const
{
    vodPlaylistData.vods.append(VMVod());
    auto& vod = vodPlaylistData.vods.last();
    auto& vodData = vod.data();
    auto vodDuration = root.value("duration").toInt();
    vodData.durationS = vodDuration;
    vodData.playlistIndex = root.value("playlist_index").toInt();
    vodData.webPageUrl = root.value(QStringLiteral("webpage_url")).toString();
    vodData.thumbnailUrl = root.value(QStringLiteral("thumbnail")).toString();
    vodData.id = root.value(QStringLiteral("id")).toString();
    vodData.title = root.value(QStringLiteral("title")).toString();
    vodData.fullTitle = root.value(QStringLiteral("fulltitle")).toString();

    QJsonArray formats = root.value(QStringLiteral("formats")).toArray();
    if (formats.isEmpty()) {
        return false;
    }

    QVector<int> videoIndices;
    videoIndices.reserve(formats.size());
    vodData.avFormats.reserve(formats.size());
    vodData.videoFormats.reserve(formats.size());
    vodData.audioFormats.reserve(formats.size());
    qDebug() << "#formats in JSON" << formats.size();
    for (int i = 0; i < formats.size(); ++i) {
        QJsonObject format = formats[i].toObject();
        QString vcodec = format.value(QStringLiteral("vcodec")).toString();
        QString acodec = format.value(QStringLiteral("acodec")).toString();

        auto hasVideo = !vcodec.isEmpty() && QStringLiteral("none") != vcodec;
        auto hasAudio = !acodec.isEmpty() && QStringLiteral("none") != acodec;

        if (hasVideo) {
            videoIndices << i;
            if (hasAudio) {
                appendAvFormat(vodData, format);
            } else {
                appendVideoFormat(vodData, format);
            }
        } else {
            if (hasAudio) {
                appendAudioFormat(vodData, format);
            }
        }
    }

    if (vodData.videoFormats.isEmpty()) {
        if (formats.isEmpty()) {
            return false;
        } else {
            if (videoIndices.isEmpty()) {
                // assume they are all valid matches
                videoIndices.resize(formats.size());
                for (int i = 0; i < videoIndices.size(); ++i) {
                    videoIndices[i] = i;
                }
            }

            // pick up all remaining videos
            for (int i = 0; i < videoIndices.size(); ++i) {
                auto formatIndex = videoIndices[i];
                QJsonObject format = formats[formatIndex].toObject();
                appendAvFormat(vodData, format);
            }
        }
    }

    return true;
}

void
VMYTDL::onPlaylistProcessFinished(int code, QProcess::ExitStatus status)
{
    QProcess* process = qobject_cast<QProcess*>(QObject::sender());
    Q_ASSERT(process);

    cleanupProcess(process);

    qDebug() << VM_GEN_YOUTUBE_DL_NAME " vod file process pid" << process->pid()
             << "finished, status:" << status
             << ", exit code:" << code;

    auto result = m_ProcessMap.value(process);
    m_ProcessMap.remove(process);
    const auto id = qvariant_cast<qint64>(result[s_Token]);

    auto download = qvariant_cast<VMPlaylistDownload>(result[s_Download]);
    auto& downLoadData = download.data();

    auto processError = QString::fromUtf8(process->readAllStandardError());
    auto errorLines = processError.splitRef(QChar('\n'), QString::SkipEmptyParts);
    for (int i = 0; i < errorLines.size(); ++i) {
        const auto& line = errorLines[i];
        qDebug() << "stderr" << line;
        if (line.indexOf(QStringLiteral("ERROR:")) == 0) {
            if (line.indexOf(QStringLiteral("[Errno -2]"), 0, Qt::CaseInsensitive) >= 0) {
                // ERROR: unable to download video data: <urlopen error [Errno -2] Name or service not known>
                downLoadData.error = VMVodEnums::VM_ErrorNetworkDown;
            } else if (line.indexOf(QStringLiteral("[Errno -3]"), 0, Qt::CaseInsensitive) >= 0) {
                // "ERROR: Failed to download m3u8 information: <urlopen error [Errno -3] Temporary failure in name resolution> (caused by URLError(gaierror(-3, 'Temporary failure in name resolution')))"
                downLoadData.error = VMVodEnums::VM_ErrorTemporaryFailureNameResolution;
            } else if (line.indexOf(QStringLiteral("[Errno 28]"), 0, Qt::CaseInsensitive) >= 0) {
                // ERROR: unable to write data: [Errno 28] No space left on device
                download.data().error = VMVodEnums::VM_ErrorNoSpaceLeftOnDevice;
            } else if (line.indexOf(QStringLiteral("timed out"), 0, Qt::CaseInsensitive) >= 0) {
                // stderr "ERROR: unable to download video data: <urlopen error _ssl.c:584: The handshake operation timed out>"
                downLoadData.error = VMVodEnums::VM_ErrorTimedOut;
            } else if (line.indexOf(QStringLiteral("giving up after"), 0, Qt::CaseInsensitive) >= 0) {
                // stderr "ERROR: giving up after 10 retries"
                downLoadData.error = VMVodEnums::VM_ErrorTimedOut;
            } else if (line.indexOf(QStringLiteral("requested format not available"), 0, Qt::CaseInsensitive) >= 0) {
                downLoadData.error = VMVodEnums::VM_ErrorFormatNotAvailable;
            } else if (line.indexOf(QStringLiteral("Forbidden"), 0, Qt::CaseInsensitive) >= 0) {
                // ERROR: unable to download video data: HTTP Error 403: Forbidden"
                downLoadData.error = VMVodEnums::VM_ErrorAccess;
            } else if (line.indexOf(QStringLiteral("Requested format is not available"), 0, Qt::CaseInsensitive) >= 0) {
                //  ERROR: [generic] www.reddit: Requested format is not available
                downLoadData.error = VMVodEnums::VM_ErrorFormatNotAvailable;
            } else {
                downLoadData.error = VMVodEnums::VM_ErrorUnknown;
            }

            downLoadData.errorMessage = line.toString();
            emit playlistDownloadCompleted(id, download);
            return;
        }
    }

    downLoadData.progress = 1;

    qDebug() << id << download;
    emit playlistDownloadCompleted(id, download);
}

void
VMYTDL::onProcessError(QProcess::ProcessError error)
{
    QProcess* process = qobject_cast<QProcess*>(QObject::sender());
    Q_ASSERT(process);

    qDebug() << "process pid" << process->pid() << "error" << error;

    cleanupProcess(process);


    QVariantMap result = m_ProcessMap.value(process);
    m_ProcessMap.remove(process);
    qint64 id = qvariant_cast<qint64>(result[s_Token]);
    m_VodDownloads.remove(id);


    if (result[s_Type] == s_MetaData) {
        auto download = qvariant_cast<VMMetaDataDownload>(result[s_Download]);
        if (download.error() != VMVodEnums::VM_ErrorCanceled) {
            switch (error) {
            case QProcess::Crashed:
                download.data().error = VMVodEnums::VM_ErrorCrashed;
                download.data().errorMessage = QStringLiteral("process path=%1 pid=%2 crashed").arg(m_YoutubeDl_Path, process->pid());
                break;
            case QProcess::Timedout:
                download.data().error = VMVodEnums::VM_ErrorTimedOut;
                download.data().errorMessage = process->errorString();
                break;
            default:
                download.data().error = VMVodEnums::VM_ErrorUnknown;
                download.data().errorMessage = process->errorString();
                break;
            }
        }
        emit metaDataDownloadCompleted(id, download);
    } else {
        auto download = qvariant_cast<VMPlaylistDownload>(result[s_Download]);
        auto& downloadData = download.data();
        if (downloadData.error != VMVodEnums::VM_ErrorCanceled) {
            switch (error) {
            case QProcess::Crashed:
                downloadData.error = VMVodEnums::VM_ErrorCrashed;
                downloadData.errorMessage = QStringLiteral("process path=%1 pid=%2 crashed").arg(m_YoutubeDl_Path, process->pid());
                break;
            case QProcess::Timedout:
                downloadData.error = VMVodEnums::VM_ErrorTimedOut;
                downloadData.errorMessage = process->errorString();
                break;
            default:
                downloadData.error = VMVodEnums::VM_ErrorUnknown;
                downloadData.errorMessage = process->errorString();
                break;
            }
        }
        emit playlistDownloadCompleted(id, download);
    }
}

void
VMYTDL::onYoutubeDlVodFileDownloadProcessReadReady()
{
    QProcess* process = qobject_cast<QProcess*>(QObject::sender());
    Q_ASSERT(process);

    if (!m_ProcessMap.contains(process)) {
        qDebug() << "process pid" << process->pid() << "gone";
        return;
    }

    const QVariantMap result = m_ProcessMap.value(process);
    qint64 id = qvariant_cast<qint64>(result[s_Token]);
    auto download = qvariant_cast<VMPlaylistDownload>(result[s_Download]);
    auto& downloadData = download.data();
    auto indices = qvariant_cast<IndexList>(result[s_Indices]);
    auto duration = result[s_Duration].toInt();


    QByteArray data = process->readAll();
    int start = 0, end = 0;
    while (end < data.size()) {
        if (data[end] == '\r' || data[end] == '\n') {
            if (start < end) {
                QString str = QString::fromUtf8(data.data() + start, end - start);
                ParseYtdlOutput(str, downloadData, indices, duration);
            }

            start = ++end;
        } else {
            ++end;
        }
    }

    if (start < end) {
        QString str = QString::fromUtf8(data.data() + start, end - start);
        ParseYtdlOutput(str, downloadData, indices, duration);
    }

    qDebug() << "process pid" << process->pid()
             << "current file index" << downloadData.currentFileIndex
             << "playlist index" << downloadData.files[downloadData.currentFileIndex].playlistIndex()
             << "progress" << downloadData.progress;

    // update file size
    auto& vodFileData = downloadData.files[downloadData.currentFileIndex].data();
    downloadData.fileSize -= vodFileData.fileSize;
    QFileInfo fi(vodFileData.filePath);
    if (fi.exists()) {
        vodFileData.fileSize = fi.size();
    } else {
        vodFileData.fileSize = 0;
    }
    downloadData.fileSize += vodFileData.fileSize;


    // update changed
    downloadData.timeChanged = QDateTime::currentDateTime();

    emit playlistDownloadChanged(id, download);
}

void
VMYTDL::cleanupProcess(QProcess* process) {
    Q_ASSERT(process);
    process->disconnect();
    process->deleteLater();
}

QVariantList
VMYTDL::inProgressPlaylistFetches() {

    QVariantList result;
    result.reserve(m_VodDownloads.size());
    foreach(QProcess* process, m_VodDownloads.values()) {
        result << m_ProcessMap[process];
    }

    return result;
}


void
VMYTDL::fillFormatId(VMVideoFormatData& f) const
{
    VMVodEnums::Format format = VMVodEnums::VM_Unknown;

    auto value = f.height;
    if (value > 0) {
        if (value <= 160) {
            format = VMVodEnums::VM_160p;
        } else if (value <= 240) {
            format = VMVodEnums::VM_240p;
        } else if (value <= 360) {
            format = VMVodEnums::VM_360p;
        } else if (value <= 480) {
            format = VMVodEnums::VM_480p;
        } else if (value <= 720) {
            format = VMVodEnums::VM_720p;
        } else if (value <= 1080) {
            format = VMVodEnums::VM_1080p;
        } else if (value <= 1440) {
            format = VMVodEnums::VM_1440p;
        } else {
            format = VMVodEnums::VM_2160p;
        }
    }

    f.format = format;
}

void
VMYTDL::fillWidth(VMVideoFormatData& format) const
{
    if (0 == format.width && format.format != VMVodEnums::VM_Unknown) {
        switch (format.format) {
        case VMVodEnums::VM_160p:
            format.width = 280;
            break;
        case VMVodEnums::VM_240p:
            format.width = 320;
            break;
        case VMVodEnums::VM_360p:
            format.width = 480;
            break;
        case VMVodEnums::VM_480p:
            format.width = 640;
            break;
        case VMVodEnums::VM_720p:
            format.width = 1280;
            break;
        case VMVodEnums::VM_1080p:
            format.width = 1920;
            break;
        case VMVodEnums::VM_1440p:
            format.width = 2560;
            break;
        case VMVodEnums::VM_2160p:
            format.width = 3840;
            break;
        }
    }
}

VMYTDL::Normalizer
VMYTDL::setUrlNormalizer(Normalizer&& n)
{
    auto result = std::move(m_Normalizer);
    m_Normalizer = std::move(n);
    return result;
}


QProcess*
VMYTDL::createProcess()
{
    QProcess* process = new QProcess(this);
    auto env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("LC_ALL"), QStringLiteral("en_US.UTF-8"));
    process->setProcessEnvironment(env);
    return process;
}

bool
VMYTDL::parseJson(const QByteArray& bytes, QVector<int>* starts, QVector<int>* ends)
{
    QList<char> scopeStack;
    Q_ASSERT(starts);
    Q_ASSERT(ends);

    const int end = bytes.size();

    // When given the --verbose flag youtube-dl will
    // prepend some debug information to each JSON meta data
    int start = 0;
    while (true) {
    //    while (start < end) {
    //        if (0 == strncmp("[debug] ", &bytes[start], 8)) {
    //            start += 8;
    //            // skip to end of line
    //            while (start < end && !(bytes[start] == '\n' || bytes[start] == '\r')) {
    //                ++start;
    //            }

    //            // skip newline
    //            while (start < end && (bytes[start] == '\n' || bytes[start] == '\r')) {
    //                ++start;
    //            }
    //        }
    //    }



        while (start < end && bytes[start] != '{') {
            // skip to end of line
            while (start < end && !(bytes[start] == '\n' || bytes[start] == '\r')) {
                ++start;
            }

            // skip newline
            while (start < end && (bytes[start] == '\n' || bytes[start] == '\r')) {
                ++start;
            }
        }

        if (start == end) {
            break;
        }

        for (int it = start; it < end; ++it) {
            auto c = bytes[it];
            if (scopeStack.isEmpty()) {
                switch (c) {
                case '{':
                    starts->push_back(it);
                    scopeStack.push_back(c);
                    break;
                default:
                    qDebug("expected { at start of JSON at index %d\n", it);
                    return false;
                }
            } else {
                switch (c) {
                case '{':
                case '[':
                    scopeStack.push_back(c);
                    break;
                case '}':
                case ']':
                    if (scopeStack.isEmpty()) {
                        qDebug("unbalanced %c in JSON document at index %d\n", c, it);
                        return false;
                    }

                    if ((scopeStack.back() == '{' && c != '}') ||
                            (scopeStack.back() == '[' && c != ']')) {
                        qDebug("closing scope type mismatch %c in JSON document at index %d\n", c, it);
                        return false;
                    }

                    scopeStack.pop_back();
                    if (scopeStack.isEmpty()) {
                        ends->push_back(it+1);
                        start = it + 1;
                        it = end;
                    }
                    break;
                default:
                    // ignore non-scope chars
                    break;
                }
            }
        }
    }

    if (ends->empty()) {
        qDebug("failed to find opening '{' of toplevel JSON object\n");
        return false;
    }

    if (starts->size() != ends->size()) {
        qDebug("no closing '}' of toplevel JSON object found\n");
        return false;
    }

    return true;
}

void
VMYTDL::setCustomYtdlOptions(const QStringList& value)
{
    m_CustomOptions = value;
    emit customYtdlOptionsChanged();
}

void
VMYTDL::setYtdlVerbose(bool value)
{
    if (value != m_YtdlVerbose) {
        m_YtdlVerbose = value;
        emit ytdlVerboseChanged();
    }
}

void
VMYTDL::setCacheDirectory(const QString& value)
{
    if (value != m_CacheDirectory) {
        m_CacheDirectory = value;
        emit ytdlVerboseChanged();
    }
}

