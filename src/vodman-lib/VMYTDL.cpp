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

#include "VMYTDL.h"
#include "VMVod.h"
#include "VMVodFileDownload.h"
#include "VMVodMetaDataDownload.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>
#include <QProcess>
#include <QDebug>
#include <QProcess>
#include <QRegExp>
#include <QUrlQuery>
#include <QFile>
#include <QFileInfo>

namespace {

const QRegExp s_YTDLProgressRegexp("\\[download\\]\\s+(\\d+\\.\\d*)%\\s+");
// 160p30
const QRegExp s_FramerateRegexp("\\d+p(\\d+)");
//const QRegExp s_CurlProgressRegexp("^.*(\\d+\\.\\d*)%$");

const QString s_Token = QStringLiteral("token");
const QString s_Type = QStringLiteral("type");
const QString s_MetaData = QStringLiteral("metadata");
const QString s_Download = QStringLiteral("download");
const QString s_NoCallHome = QStringLiteral("--no-call-home");

void Nop(QString&) {}

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
VMYTDL::startFetchVodMetaData(qint64 token, const QString& _url) {

    VMVodMetaDataDownload download;
    VMVodMetaDataDownloadData& data = download.data();

    QString url(_url);
    m_Normalizer(url);

    data._url = url;
    if (!available()) {
        data.errorMessage = QStringLiteral("path to youtube-dl no set");
        data.error = VMVodEnums::VM_ErrorNoYoutubeDl;
        qDebug() << data.errorMessage;
        emit vodMetaDataDownloadCompleted(token, download);
        return;
    }

    qDebug() << "Trying to obtain video urls for: " << url;

    auto cacheEntryPtr = m_MetaDataCache[download.data()._url];
    if (cacheEntryPtr) {
        auto now = QDateTime::currentDateTime();
        if (m_MetaDataSecondsValid < 0 || now <= cacheEntryPtr->fetchTime.addSecs(m_MetaDataSecondsValid)) {
            qDebug() << "Response for" << download.data()._url << "available in cache, using it";
            data._vod = cacheEntryPtr->vod;
            data.error = VMVodEnums::VM_ErrorNone;
            emit vodMetaDataDownloadCompleted(token, download);
            return;
        }

        m_MetaDataCache.remove(download.data()._url);
    }

    QVariantMap result;
    result[s_Type] = s_MetaData;
    result[s_Download] = QVariant::fromValue(download);
    result[s_Token] = token;

    QStringList arguments;
    arguments << QStringLiteral("--dump-json")
              << QStringLiteral("--youtube-skip-dash-manifest")
              << QStringLiteral("--no-cache-dir")
              << s_NoCallHome
              << url;

    qDebug() << "youtube-dl subprocess:" << m_YoutubeDl_Path << arguments;

    auto process = createProcess();
    connect(process, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(onMetaDataProcessFinished(int, QProcess::ExitStatus)));
    connect(process, SIGNAL(error(QProcess::ProcessError)),
            this, SLOT(onProcessError(QProcess::ProcessError)));

    m_ProcessMap.insert(process, result);

    process->start(m_YoutubeDl_Path, arguments, QIODevice::ReadOnly);
    return;
}


void
VMYTDL::startFetchVodFile(qint64 token, const VMVodFileDownloadRequest& req, VMVodFileDownload* _download)
{
    VMVodFileDownload download;

    if (!_download) {
        _download = &download;
    }

    VMVodFileDownloadData& data = _download->data();
    data._filePath = req.filePath;
    data.format = req.format;
    data.description = req.description;
    data.timeStarted = data.timeChanged = QDateTime::currentDateTime();

    if (!available()) {
        data.errorMessage = QStringLiteral("path to youtube-dl no set");
        data.error = VMVodEnums::VM_ErrorNoYoutubeDl;
        qDebug() << data.errorMessage;
        emit vodFileDownloadCompleted(token, *_download);
        return;
    }


    QVariantMap result;

    result[s_Type] = QStringLiteral("vod");
    result[s_Download] = QVariant::fromValue(*_download);
    result[s_Token] = token;

    qDebug() << "Trying to obtain video file for: " << req.format.vodUrl() << "format:" << req.format.id() << "file path:" << req.filePath;

    // youtube-dl -f 160p30 https://www.twitch.tv/videos/161472611?t=07h49m09s --no-cache-dir --no-call-home --newline -o foo.bar

    QStringList arguments;
    arguments << QStringLiteral("-c")
              << QStringLiteral("--newline")
              << QStringLiteral("--no-part")
              << QStringLiteral("--no-cache-dir")
              << s_NoCallHome
              << QStringLiteral("-f") << req.format.id()
              << QStringLiteral("-o") << req.filePath
              << req.format.vodUrl();

    qDebug() << "youtube-dl subprocess:" << m_YoutubeDl_Path << arguments;

    QProcess* process = createProcess();
    connect(process, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(onVodFileProcessFinished(int,QProcess::ExitStatus)));
    connect(process, &QProcess::readyReadStandardOutput,
            this, &VMYTDL::onYoutubeDlVodFileDownloadProcessReadReady);
    connect(process, SIGNAL(errorOccurred(QProcess::ProcessError)),
            this, SLOT(onProcessError(QProcess::ProcessError)));


    m_ProcessMap.insert(process, result);
    m_VodDownloads.insert(token, process);

    process->start(m_YoutubeDl_Path, arguments, QIODevice::ReadOnly);
}

void
VMYTDL::cancelFetchVodFile(qint64 token, bool deleteFile) {
    qDebug() << "token" << token << "delete?" << deleteFile;

    QProcess* process = m_VodDownloads.value(token);
    if (process) {
        const auto& m = m_ProcessMap[process];
        VMVodFileDownload download = qvariant_cast<VMVodFileDownload>(m[s_Download]);
        download.data().error = VMVodEnums::VM_ErrorCanceled;

        qDebug() << "kill pid" << process->processId();
        process->kill();

        if (deleteFile) {
            qDebug() << "removing" << download.filePath();
            QFile::remove(download.filePath());
        }
    }
}

void
VMYTDL::onMetaDataProcessFinished(int code, QProcess::ExitStatus status)
{
    QProcess* process = qobject_cast<QProcess*>(QObject::sender());
    Q_ASSERT(process);

    cleanupProcess(process);

    qDebug() << "youtube-dl meta data process pid" << process->processId()
             << "finished, status:" << status
             << ", exit code:" << code;

    auto result = m_ProcessMap.value(process);
    m_ProcessMap.remove(process);

    const qint64 id = qvariant_cast<qint64>(result[s_Token]);
    VMVodMetaDataDownload download = qvariant_cast<VMVodMetaDataDownload>(result[s_Download]);
    VMVodMetaDataDownloadData& downLoadData = download.data();
    downLoadData.error = VMVodEnums::VM_ErrorNone;

    QString processError = QString::fromLocal8Bit(process->readAllStandardError());
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
            } else if (line.indexOf(QStringLiteral("is not a valid URL"), 0, Qt::CaseInsensitive) >= 0) {
                //ERROR: '' is not a valid URL. Set --default-search \"ytsearch\" (or run  youtube-dl \"ytsearch:\" ) to search YouTube
                downLoadData.error = VMVodEnums::VM_ErrorInvalidUrl;
            } else if (line.indexOf(QStringLiteral("This video is unavailable."), 0, Qt::CaseInsensitive) >= 0) {
                downLoadData.error = VMVodEnums::VM_ErrorContentGone;
            } else {
                downLoadData.error = VMVodEnums::VM_ErrorUnknown;
            }
            emit vodMetaDataDownloadCompleted(id, download);
            return;
        }
    }

    QByteArray output = process->readAllStandardOutput();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(output, &error);
    if (error.error != QJsonParseError::NoError) {
        downLoadData.error = VMVodEnums::VM_ErrorInvalidResponse;
        downLoadData.errorMessage = QStringLiteral("JSON parse error: %1").arg(error.errorString());
        qCritical() << "JSON parse error:" << error.errorString();
        emit vodMetaDataDownloadCompleted(id, download);
        return;
    }


//    // twitch {"view_count": 19484, "width": 1920, "format_id": "Source", "display_id": "v161472611", "requested_subtitles": null, "fulltitle": "HSC XV Day 4", "extractor": "twitch:vod", "protocol": "m3u8_native", "timestamp": 1500814982, "subtitles": {"rechat": [{"ext": "json", "url": "https://rechat.twitch.tv/rechat-messages?video_id=v161472611&start=1500814982"}]}, "thumbnail": "https://static-cdn.jtvnw.net/s3_vods/a490bf1a61acbc89fee6_taketv_25826129648_682803294//thumb/thumb161472611-320x240.jpg", "formats": [{"manifest_url": "https://usher.ttvnw.net/vod/161472611?allow_audio_only=true&nauth=%7B%22user_id%22%3Anull%2C%22vod_id%22%3A161472611%2C%22expires%22%3A1510105630%2C%22chansub%22%3A%7B%22restricted_bitrates%22%3A%5B%5D%7D%2C%22privileged%22%3Afalse%2C%22https_required%22%3Afalse%7D&player=twitchweb&allow_source=true&allow_spectre=true&nauthsig=cd9650b6faa761e34c77a49b777e4fde6479be58", "ext": "mp4", "format": "Audio_Only - audio only", "tbr": 218.991, "protocol": "m3u8_native", "preference": null, "fps": null, "format_id": "Audio_Only", "acodec": "mp4a.40.2", "vcodec": "none", "http_headers": {"Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Encoding": "gzip, deflate", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)"}, "url": "https://fastly.vod.hls.ttvnw.net/a490bf1a61acbc89fee6_taketv_25826129648_682803294/audio_only/highlight-161472611-muted-F7A9TIGDWS.m3u8"}, {"width": 284, "manifest_url": "https://usher.ttvnw.net/vod/161472611?allow_audio_only=true&nauth=%7B%22user_id%22%3Anull%2C%22vod_id%22%3A161472611%2C%22expires%22%3A1510105630%2C%22chansub%22%3A%7B%22restricted_bitrates%22%3A%5B%5D%7D%2C%22privileged%22%3Afalse%2C%22https_required%22%3Afalse%7D&player=twitchweb&allow_source=true&allow_spectre=true&nauthsig=cd9650b6faa761e34c77a49b777e4fde6479be58", "ext": "mp4", "format": "160p30 - 284x160", "tbr": 228.015, "protocol": "m3u8_native", "preference": null, "fps": null, "format_id": "160p30", "acodec": "mp4a.40.2", "vcodec": "avc1.4D400C", "http_headers": {"Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Encoding": "gzip, deflate", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)"}, "url": "https://fastly.vod.hls.ttvnw.net/a490bf1a61acbc89fee6_taketv_25826129648_682803294/160p30/highlight-161472611-muted-F7A9TIGDWS.m3u8", "height": 160}, {"width": 640, "manifest_url": "https://usher.ttvnw.net/vod/161472611?allow_audio_only=true&nauth=%7B%22user_id%22%3Anull%2C%22vod_id%22%3A161472611%2C%22expires%22%3A1510105630%2C%22chansub%22%3A%7B%22restricted_bitrates%22%3A%5B%5D%7D%2C%22privileged%22%3Afalse%2C%22https_required%22%3Afalse%7D&player=twitchweb&allow_source=true&allow_spectre=true&nauthsig=cd9650b6faa761e34c77a49b777e4fde6479be58", "ext": "mp4", "format": "360p30 - 640x360", "tbr": 562.609, "protocol": "m3u8_native", "preference": null, "fps": null, "format_id": "360p30", "acodec": "mp4a.40.2", "vcodec": "avc1.4D401E", "http_headers": {"Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Encoding": "gzip, deflate", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)"}, "url": "https://fastly.vod.hls.ttvnw.net/a490bf1a61acbc89fee6_taketv_25826129648_682803294/360p30/highlight-161472611-muted-F7A9TIGDWS.m3u8", "height": 360}, {"width": 852, "manifest_url": "https://usher.ttvnw.net/vod/161472611?allow_audio_only=true&nauth=%7B%22user_id%22%3Anull%2C%22vod_id%22%3A161472611%2C%22expires%22%3A1510105630%2C%22chansub%22%3A%7B%22restricted_bitrates%22%3A%5B%5D%7D%2C%22privileged%22%3Afalse%2C%22https_required%22%3Afalse%7D&player=twitchweb&allow_source=true&allow_spectre=true&nauthsig=cd9650b6faa761e34c77a49b777e4fde6479be58", "ext": "mp4", "format": "480p30 - 852x480", "tbr": 1031.06, "protocol": "m3u8_native", "preference": null, "fps": null, "format_id": "480p30", "acodec": "mp4a.40.2", "vcodec": "avc1.4D401E", "http_headers": {"Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Encoding": "gzip, deflate", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)"}, "url": "https://fastly.vod.hls.ttvnw.net/a490bf1a61acbc89fee6_taketv_25826129648_682803294/480p30/highlight-161472611-muted-F7A9TIGDWS.m3u8", "height": 480}, {"width": 1280, "manifest_url": "https://usher.ttvnw.net/vod/161472611?allow_audio_only=true&nauth=%7B%22user_id%22%3Anull%2C%22vod_id%22%3A161472611%2C%22expires%22%3A1510105630%2C%22chansub%22%3A%7B%22restricted_bitrates%22%3A%5B%5D%7D%2C%22privileged%22%3Afalse%2C%22https_required%22%3Afalse%7D&player=twitchweb&allow_source=true&allow_spectre=true&nauthsig=cd9650b6faa761e34c77a49b777e4fde6479be58", "ext": "mp4", "format": "720p30 - 1280x720", "tbr": 1705.809, "protocol": "m3u8_native", "preference": null, "fps": null, "format_id": "720p30", "acodec": "mp4a.40.2", "vcodec": "avc1.4D401F", "http_headers": {"Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Encoding": "gzip, deflate", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)"}, "url": "https://fastly.vod.hls.ttvnw.net/a490bf1a61acbc89fee6_taketv_25826129648_682803294/720p30/highlight-161472611-muted-F7A9TIGDWS.m3u8", "height": 720}, {"width": 1920, "manifest_url": "https://usher.ttvnw.net/vod/161472611?allow_audio_only=true&nauth=%7B%22user_id%22%3Anull%2C%22vod_id%22%3A161472611%2C%22expires%22%3A1510105630%2C%22chansub%22%3A%7B%22restricted_bitrates%22%3A%5B%5D%7D%2C%22privileged%22%3Afalse%2C%22https_required%22%3Afalse%7D&player=twitchweb&allow_source=true&allow_spectre=true&nauthsig=cd9650b6faa761e34c77a49b777e4fde6479be58", "ext": "mp4", "format": "Source - 1920x1080", "tbr": 6325.641, "protocol": "m3u8_native", "preference": 10, "fps": null, "format_id": "Source", "acodec": "mp4a.40.2", "vcodec": "avc1.4D402A", "http_headers": {"Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Encoding": "gzip, deflate", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)"}, "url": "https://fastly.vod.hls.ttvnw.net/a490bf1a61acbc89fee6_taketv_25826129648_682803294/chunked/highlight-161472611-muted-F7A9TIGDWS.m3u8", "height": 1080}], "vcodec": "avc1.4D402A", "manifest_url": "https://usher.ttvnw.net/vod/161472611?allow_audio_only=true&nauth=%7B%22user_id%22%3Anull%2C%22vod_id%22%3A161472611%2C%22expires%22%3A1510105630%2C%22chansub%22%3A%7B%22restricted_bitrates%22%3A%5B%5D%7D%2C%22privileged%22%3Afalse%2C%22https_required%22%3Afalse%7D&player=twitchweb&allow_source=true&allow_spectre=true&nauthsig=cd9650b6faa761e34c77a49b777e4fde6479be58", "extractor_key": "TwitchVod", "description": "", "fps": null, "height": 1080, "_filename": "HSC XV Day 4-v161472611.mp4", "uploader_id": "taketv", "title": "HSC XV Day 4", "preference": 10, "uploader": "TaKeTV", "format": "Source - 1920x1080", "start_time": 28149.0, "playlist_index": null, "duration": 33528, "acodec": "mp4a.40.2", "webpage_url_basename": "161472611", "webpage_url": "https://www.twitch.tv/videos/161472611?t=07h49m09s", "ext": "mp4", "thumbnails": [{"url": "https://static-cdn.jtvnw.net/s3_vods/a490bf1a61acbc89fee6_taketv_25826129648_682803294//thumb/thumb161472611-320x240.jpg", "id": "0"}], "http_headers": {"Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Encoding": "gzip, deflate", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)"}, "id": "v161472611", "playlist": null, "tbr": 6325.641, "url": "https://fastly.vod.hls.ttvnw.net/a490bf1a61acbc89fee6_taketv_25826129648_682803294/chunked/highlight-161472611-muted-F7A9TIGDWS.m3u8", "upload_date": "20170723"}
//    // youtube {"uploader_url": null, "series": null, "width": 1920, "webpage_url": "https://www.youtube.com/watch?v=5IwQ0AGvfuQ", "format": "299 - 1920x1080 (DASH video)+251 - audio only (DASH audio)", "extractor_key": "Youtube", "end_time": null, "ext": "mp4", "subtitles": {}, "start_time": null, "thumbnail": "https://i.ytimg.com/vi/5IwQ0AGvfuQ/default.jpg", "description": "", "height": 1080, "title": "StarCraft: Remastered - UNITED EARTH DIRECTORATE!", "categories": null, "season_number": null, "requested_subtitles": null, "requested_formats": [{"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?id=e48c10d001af7ee4&itag=299&source=youtube&requiressl=yes&mn=sn-4g5ednld&ms=au&mv=m&ei=hkcBWs3jEIvscp-5gagE&pl=17&mm=31&initcwndbps=1311250&ratebypass=yes&mime=video/mp4&gir=yes&clen=515535482&lmt=1510029653727916&dur=1205.383&key=dg_yt0&signature=1702D702941E3F03DBBA58CDF9E9F59A51031584.2B1D90096C036ECF749B843F65F4DEF4939F0667&mt=1510033130&ip=149.172.215.29&ipbits=0&expire=1510054886&sparams=ip,ipbits,expire,id,itag,source,requiressl,mn,ms,mv,ei,pl,mm,initcwndbps,ratebypass,mime,gir,clen,lmt,dur", "manifest_url": "https://manifest.googlevideo.com/api/manifest/dash/id/e48c10d001af7ee4/requiressl/yes/key/yt6/mn/sn-4g5ednld/expire/1510054886/sparams/as%2Cei%2Chfr%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Cplayback_host%2Crequiressl%2Csource%2Cexpire/source/youtube/itag/0/ms/au/playback_host/r4---sn-4g5ednld.googlevideo.com/mv/m/ei/hkcBWs3jEIvscp-5gagE/signature/E3C9CB7E639DAC1F379DD3286CFB5219BEC1DF56.16DAB28033C7AC96D8875B4DF9A3FBAEBB4E05D7/pl/17/ipbits/0/mm/31/hfr/1/initcwndbps/1311250/ip/149.172.215.29/mt/1510033130/as/fmp4_audio_clear%2Cfmp4_sd_hd_clear", "height": 1080, "protocol": "https", "format_note": "DASH video", "format": "299 - 1920x1080 (DASH video)", "language": null, "ext": "mp4", "fps": 60, "format_id": "299", "width": 1920, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "avc1.64002a", "filesize": 515535482, "tbr": 6644.16, "acodec": "none", "asr": null}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mn=sn-4g5ednld&mm=31&ip=149.172.215.29&gir=yes&pl=17&mime=audio%2Fwebm&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&mv=m&sparams=clen%2Cdur%2Cei%2Cgir%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Ckeepalive%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Crequiressl%2Csource%2Cexpire&mt=1510033130&itag=251&keepalive=yes&key=yt6&signature=63C4DCAEE40A17284F3F531D872D25281C0E4A21.680B6BEB8931E76A38D27FED5EA749478FA35B34&ei=hUcBWpj6IJWKcO6hq2g&expire=1510054885&source=youtube&dur=1205.421&clen=19152907&initcwndbps=1397500&requiressl=yes&lmt=1509220063342381&ipbits=0&ratebypass=yes", "protocol": "https", "format_note": "DASH audio", "format": "251 - audio only (DASH audio)", "ext": "webm", "format_id": "251", "player_url": null, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "none", "filesize": 19152907, "tbr": 148.79, "acodec": "opus", "abr": 160}], "like_count": null, "fulltitle": "StarCraft: Remastered - UNITED EARTH DIRECTORATE!", "upload_date": null, "extractor": "youtube", "webpage_url_basename": "5IwQ0AGvfuQ", "is_live": null, "uploader_id": null, "episode_number": null, "thumbnails": [{"id": "0", "url": "https://i.ytimg.com/vi/5IwQ0AGvfuQ/default.jpg"}], "dislike_count": null, "acodec": "opus", "alt_title": null, "formats": [{"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?id=e48c10d001af7ee4&itag=139&source=youtube&requiressl=yes&mn=sn-4g5ednld&ms=au&mv=m&ei=hkcBWs3jEIvscp-5gagE&pl=17&mm=31&initcwndbps=1311250&ratebypass=yes&mime=audio/mp4&gir=yes&clen=7170087&lmt=1510029848295527&dur=1205.533&key=dg_yt0&signature=881BF3353FB0A0D2EA8BF7CECE15D900EC765801.0F52D0863D85E7DA66B17C00AAF628CDA498EEAF&mt=1510033130&ip=149.172.215.29&ipbits=0&expire=1510054886&sparams=ip,ipbits,expire,id,itag,source,requiressl,mn,ms,mv,ei,pl,mm,initcwndbps,ratebypass,mime,gir,clen,lmt,dur", "protocol": "https", "format_note": "DASH audio", "format": "139 - audio only (DASH audio)", "ext": "m4a", "format_id": "139", "width": null, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "height": null, "asr": 22050, "fps": null, "manifest_url": "https://manifest.googlevideo.com/api/manifest/dash/id/e48c10d001af7ee4/requiressl/yes/key/yt6/mn/sn-4g5ednld/expire/1510054886/sparams/as%2Cei%2Chfr%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Cplayback_host%2Crequiressl%2Csource%2Cexpire/source/youtube/itag/0/ms/au/playback_host/r4---sn-4g5ednld.googlevideo.com/mv/m/ei/hkcBWs3jEIvscp-5gagE/signature/E3C9CB7E639DAC1F379DD3286CFB5219BEC1DF56.16DAB28033C7AC96D8875B4DF9A3FBAEBB4E05D7/pl/17/ipbits/0/mm/31/hfr/1/initcwndbps/1311250/ip/149.172.215.29/mt/1510033130/as/fmp4_audio_clear%2Cfmp4_sd_hd_clear", "language": null, "container": "m4a_dash", "vcodec": "none", "filesize": 7170087, "tbr": 49.685, "acodec": "mp4a.40.5", "abr": 48}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mn=sn-4g5ednld&mm=31&ip=149.172.215.29&gir=yes&pl=17&mime=audio%2Fwebm&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&mv=m&sparams=clen%2Cdur%2Cei%2Cgir%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Ckeepalive%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Crequiressl%2Csource%2Cexpire&mt=1510033130&itag=249&keepalive=yes&key=yt6&signature=5E03B259A79D263CC941BB818AD76D1392494842.4A7F2C91E3A2D45A4A33444D024549E900AA58FE&ei=hUcBWpj6IJWKcO6hq2g&expire=1510054885&source=youtube&dur=1205.421&clen=7512109&initcwndbps=1397500&requiressl=yes&lmt=1509220041875458&ipbits=0&ratebypass=yes", "protocol": "https", "format_note": "DASH audio", "format": "249 - audio only (DASH audio)", "ext": "webm", "format_id": "249", "player_url": null, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "none", "filesize": 7512109, "tbr": 57.775, "acodec": "opus", "abr": 50}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mn=sn-4g5ednld&mm=31&ip=149.172.215.29&gir=yes&pl=17&mime=audio%2Fwebm&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&mv=m&sparams=clen%2Cdur%2Cei%2Cgir%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Ckeepalive%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Crequiressl%2Csource%2Cexpire&mt=1510033130&itag=250&keepalive=yes&key=yt6&signature=2C888AF2F17AFAFB706727068DB3F3FDF8AF1D09.480541EE5E5FD6DCC989A467EC48680DDE0030EE&ei=hUcBWpj6IJWKcO6hq2g&expire=1510054885&source=youtube&dur=1205.421&clen=9800616&initcwndbps=1397500&requiressl=yes&lmt=1509220081018978&ipbits=0&ratebypass=yes", "protocol": "https", "format_note": "DASH audio", "format": "250 - audio only (DASH audio)", "ext": "webm", "format_id": "250", "player_url": null, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "none", "filesize": 9800616, "tbr": 78.232, "acodec": "opus", "abr": 70}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mn=sn-4g5ednld&mm=31&ip=149.172.215.29&gir=yes&pl=17&mime=audio%2Fwebm&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&mv=m&sparams=clen%2Cdur%2Cei%2Cgir%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Ckeepalive%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Crequiressl%2Csource%2Cexpire&mt=1510033130&itag=171&keepalive=yes&key=yt6&signature=579E5C3854EFE77C5EF5A7B7D0562DA1740248FA.53A87A7BBFF2E0FAAB1432926DA057A74D2FB9FF&ei=hUcBWpj6IJWKcO6hq2g&expire=1510054885&source=youtube&dur=1205.389&clen=15461858&initcwndbps=1397500&requiressl=yes&lmt=1509220115324822&ipbits=0&ratebypass=yes", "protocol": "https", "format_note": "DASH audio", "format": "171 - audio only (DASH audio)", "ext": "webm", "format_id": "171", "player_url": null, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "none", "filesize": 15461858, "tbr": 120.117, "acodec": "vorbis", "abr": 128}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?id=e48c10d001af7ee4&itag=140&source=youtube&requiressl=yes&mn=sn-4g5ednld&ms=au&mv=m&ei=hkcBWs3jEIvscp-5gagE&pl=17&mm=31&initcwndbps=1311250&ratebypass=yes&mime=audio/mp4&gir=yes&clen=19146176&lmt=1510029849797209&dur=1205.440&key=dg_yt0&signature=2BFB0E3C233101160B6524DFFE26D17ADF2EB1DA.995E6EA9287F98B9ED86A7CBA773F37159415E42&mt=1510033130&ip=149.172.215.29&ipbits=0&expire=1510054886&sparams=ip,ipbits,expire,id,itag,source,requiressl,mn,ms,mv,ei,pl,mm,initcwndbps,ratebypass,mime,gir,clen,lmt,dur", "protocol": "https", "format_note": "DASH audio", "format": "140 - audio only (DASH audio)", "ext": "m4a", "format_id": "140", "width": null, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "height": null, "asr": 44100, "fps": null, "manifest_url": "https://manifest.googlevideo.com/api/manifest/dash/id/e48c10d001af7ee4/requiressl/yes/key/yt6/mn/sn-4g5ednld/expire/1510054886/sparams/as%2Cei%2Chfr%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Cplayback_host%2Crequiressl%2Csource%2Cexpire/source/youtube/itag/0/ms/au/playback_host/r4---sn-4g5ednld.googlevideo.com/mv/m/ei/hkcBWs3jEIvscp-5gagE/signature/E3C9CB7E639DAC1F379DD3286CFB5219BEC1DF56.16DAB28033C7AC96D8875B4DF9A3FBAEBB4E05D7/pl/17/ipbits/0/mm/31/hfr/1/initcwndbps/1311250/ip/149.172.215.29/mt/1510033130/as/fmp4_audio_clear%2Cfmp4_sd_hd_clear", "language": null, "container": "m4a_dash", "vcodec": "none", "filesize": 19146176, "tbr": 129.139, "acodec": "mp4a.40.2", "abr": 128}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mn=sn-4g5ednld&mm=31&ip=149.172.215.29&gir=yes&pl=17&mime=audio%2Fwebm&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&mv=m&sparams=clen%2Cdur%2Cei%2Cgir%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Ckeepalive%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Crequiressl%2Csource%2Cexpire&mt=1510033130&itag=251&keepalive=yes&key=yt6&signature=63C4DCAEE40A17284F3F531D872D25281C0E4A21.680B6BEB8931E76A38D27FED5EA749478FA35B34&ei=hUcBWpj6IJWKcO6hq2g&expire=1510054885&source=youtube&dur=1205.421&clen=19152907&initcwndbps=1397500&requiressl=yes&lmt=1509220063342381&ipbits=0&ratebypass=yes", "protocol": "https", "format_note": "DASH audio", "format": "251 - audio only (DASH audio)", "ext": "webm", "format_id": "251", "player_url": null, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "none", "filesize": 19152907, "tbr": 148.79, "acodec": "opus", "abr": 160}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mn=sn-4g5ednld&mm=31&ip=149.172.215.29&gir=yes&pl=17&aitags=133%2C134%2C135%2C136%2C137%2C160%2C242%2C243%2C244%2C247%2C248%2C278%2C298%2C299%2C302%2C303&mime=video%2Fwebm&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&mv=m&sparams=aitags%2Cclen%2Cdur%2Cei%2Cgir%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Ckeepalive%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Crequiressl%2Csource%2Cexpire&mt=1510033130&itag=278&keepalive=yes&key=yt6&signature=97854548DE12D3E7407A9FDDB39B82BFE4513327.0D03B81AD8F85F74055764AA18F86CFD72FEA551&ei=hUcBWpj6IJWKcO6hq2g&expire=1510054885&source=youtube&dur=1205.400&clen=12758731&initcwndbps=1397500&requiressl=yes&lmt=1509220811525506&ipbits=0&ratebypass=yes", "filesize": 12758731, "protocol": "https", "format_note": "144p", "format": "278 - 256x144 (144p)", "container": "webm", "ext": "webm", "fps": 30, "format_id": "278", "player_url": null, "width": 256, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "vp9", "height": 144, "tbr": 97.225, "acodec": "none"}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?id=e48c10d001af7ee4&itag=160&source=youtube&requiressl=yes&mn=sn-4g5ednld&ms=au&mv=m&ei=hkcBWs3jEIvscp-5gagE&pl=17&mm=31&initcwndbps=1311250&ratebypass=yes&mime=video/mp4&gir=yes&clen=7406322&lmt=1510029659931569&dur=1205.399&key=dg_yt0&signature=1CF2E5077EBD96490D5E2EFD8B9866A33C51ADC4.677565442F488BE306AFA7D124A660AD5E8BFEEC&mt=1510033130&ip=149.172.215.29&ipbits=0&expire=1510054886&sparams=ip,ipbits,expire,id,itag,source,requiressl,mn,ms,mv,ei,pl,mm,initcwndbps,ratebypass,mime,gir,clen,lmt,dur", "protocol": "https", "format_note": "DASH video", "format": "160 - 256x144 (DASH video)", "ext": "mp4", "format_id": "160", "width": 256, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "height": 144, "fps": 30, "manifest_url": "https://manifest.googlevideo.com/api/manifest/dash/id/e48c10d001af7ee4/requiressl/yes/key/yt6/mn/sn-4g5ednld/expire/1510054886/sparams/as%2Cei%2Chfr%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Cplayback_host%2Crequiressl%2Csource%2Cexpire/source/youtube/itag/0/ms/au/playback_host/r4---sn-4g5ednld.googlevideo.com/mv/m/ei/hkcBWs3jEIvscp-5gagE/signature/E3C9CB7E639DAC1F379DD3286CFB5219BEC1DF56.16DAB28033C7AC96D8875B4DF9A3FBAEBB4E05D7/pl/17/ipbits/0/mm/31/hfr/1/initcwndbps/1311250/ip/149.172.215.29/mt/1510033130/as/fmp4_audio_clear%2Cfmp4_sd_hd_clear", "language": null, "vcodec": "avc1.4d400c", "filesize": 7406322, "tbr": 111.559, "acodec": "none", "asr": null}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mn=sn-4g5ednld&mm=31&ip=149.172.215.29&gir=yes&pl=17&aitags=133%2C134%2C135%2C136%2C137%2C160%2C242%2C243%2C244%2C247%2C248%2C278%2C298%2C299%2C302%2C303&mime=video%2Fwebm&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&mv=m&sparams=aitags%2Cclen%2Cdur%2Cei%2Cgir%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Ckeepalive%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Crequiressl%2Csource%2Cexpire&mt=1510033130&itag=242&keepalive=yes&key=yt6&signature=82A62FC3B3E86E188D40E8A6170A3D911E6AFDE2.9B2F792A2CC2E930F7259CE6FD1EB6874C4A2F84&ei=hUcBWpj6IJWKcO6hq2g&expire=1510054885&source=youtube&dur=1205.400&clen=25918864&initcwndbps=1397500&requiressl=yes&lmt=1509220810865334&ipbits=0&ratebypass=yes", "height": 240, "format_note": "240p", "format": "242 - 426x240 (240p)", "protocol": "https", "ext": "webm", "fps": 30, "format_id": "242", "player_url": null, "width": 426, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "vp9", "filesize": 25918864, "tbr": 225.081, "acodec": "none"}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?id=e48c10d001af7ee4&itag=133&source=youtube&requiressl=yes&mn=sn-4g5ednld&ms=au&mv=m&ei=hkcBWs3jEIvscp-5gagE&pl=17&mm=31&initcwndbps=1311250&ratebypass=yes&mime=video/mp4&gir=yes&clen=14818821&lmt=1510029659970337&dur=1205.399&key=dg_yt0&signature=16C5A8DA2D78001CCC0CD6BE6F85662A4B7C6D0D.6FF174121D9B21E48A9D377F422FF797456F8FE5&mt=1510033130&ip=149.172.215.29&ipbits=0&expire=1510054886&sparams=ip,ipbits,expire,id,itag,source,requiressl,mn,ms,mv,ei,pl,mm,initcwndbps,ratebypass,mime,gir,clen,lmt,dur", "protocol": "https", "format_note": "DASH video", "format": "133 - 426x240 (DASH video)", "ext": "mp4", "format_id": "133", "width": 426, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "height": 240, "fps": 30, "manifest_url": "https://manifest.googlevideo.com/api/manifest/dash/id/e48c10d001af7ee4/requiressl/yes/key/yt6/mn/sn-4g5ednld/expire/1510054886/sparams/as%2Cei%2Chfr%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Cplayback_host%2Crequiressl%2Csource%2Cexpire/source/youtube/itag/0/ms/au/playback_host/r4---sn-4g5ednld.googlevideo.com/mv/m/ei/hkcBWs3jEIvscp-5gagE/signature/E3C9CB7E639DAC1F379DD3286CFB5219BEC1DF56.16DAB28033C7AC96D8875B4DF9A3FBAEBB4E05D7/pl/17/ipbits/0/mm/31/hfr/1/initcwndbps/1311250/ip/149.172.215.29/mt/1510033130/as/fmp4_audio_clear%2Cfmp4_sd_hd_clear", "language": null, "vcodec": "avc1.4d4015", "filesize": 14818821, "tbr": 245.045, "acodec": "none", "asr": null}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mn=sn-4g5ednld&mm=31&ip=149.172.215.29&gir=yes&pl=17&aitags=133%2C134%2C135%2C136%2C137%2C160%2C242%2C243%2C244%2C247%2C248%2C278%2C298%2C299%2C302%2C303&mime=video%2Fwebm&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&mv=m&sparams=aitags%2Cclen%2Cdur%2Cei%2Cgir%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Ckeepalive%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Crequiressl%2Csource%2Cexpire&mt=1510033130&itag=243&keepalive=yes&key=yt6&signature=3E874ACE17843E53F22764FBAF7CF68B712F4DC9.1B139B947897785AC0E736AD5A26ED0EAD4462ED&ei=hUcBWpj6IJWKcO6hq2g&expire=1510054885&source=youtube&dur=1205.400&clen=49665778&initcwndbps=1397500&requiressl=yes&lmt=1509220812356164&ipbits=0&ratebypass=yes", "height": 360, "format_note": "360p", "format": "243 - 640x360 (360p)", "protocol": "https", "ext": "webm", "fps": 30, "format_id": "243", "player_url": null, "width": 640, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "vp9", "filesize": 49665778, "tbr": 415.981, "acodec": "none"}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?id=e48c10d001af7ee4&itag=134&source=youtube&requiressl=yes&mn=sn-4g5ednld&ms=au&mv=m&ei=hkcBWs3jEIvscp-5gagE&pl=17&mm=31&initcwndbps=1311250&ratebypass=yes&mime=video/mp4&gir=yes&clen=40277006&lmt=1510029659922435&dur=1205.399&key=dg_yt0&signature=6F320795FBD46227C824B218031591BDA3A07A0C.766E6873293147D07518D0317C7025DC3B1490D9&mt=1510033130&ip=149.172.215.29&ipbits=0&expire=1510054886&sparams=ip,ipbits,expire,id,itag,source,requiressl,mn,ms,mv,ei,pl,mm,initcwndbps,ratebypass,mime,gir,clen,lmt,dur", "protocol": "https", "format_note": "DASH video", "format": "134 - 640x360 (DASH video)", "ext": "mp4", "format_id": "134", "width": 640, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "height": 360, "fps": 30, "manifest_url": "https://manifest.googlevideo.com/api/manifest/dash/id/e48c10d001af7ee4/requiressl/yes/key/yt6/mn/sn-4g5ednld/expire/1510054886/sparams/as%2Cei%2Chfr%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Cplayback_host%2Crequiressl%2Csource%2Cexpire/source/youtube/itag/0/ms/au/playback_host/r4---sn-4g5ednld.googlevideo.com/mv/m/ei/hkcBWs3jEIvscp-5gagE/signature/E3C9CB7E639DAC1F379DD3286CFB5219BEC1DF56.16DAB28033C7AC96D8875B4DF9A3FBAEBB4E05D7/pl/17/ipbits/0/mm/31/hfr/1/initcwndbps/1311250/ip/149.172.215.29/mt/1510033130/as/fmp4_audio_clear%2Cfmp4_sd_hd_clear", "language": null, "vcodec": "avc1.4d401e", "filesize": 40277006, "tbr": 635.454, "acodec": "none", "asr": null}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mn=sn-4g5ednld&mm=31&ip=149.172.215.29&gir=yes&pl=17&aitags=133%2C134%2C135%2C136%2C137%2C160%2C242%2C243%2C244%2C247%2C248%2C278%2C298%2C299%2C302%2C303&mime=video%2Fwebm&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&mv=m&sparams=aitags%2Cclen%2Cdur%2Cei%2Cgir%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Ckeepalive%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Crequiressl%2Csource%2Cexpire&mt=1510033130&itag=244&keepalive=yes&key=yt6&signature=CD835C97A6B63AA40DC139A77CD79F8D094A1ED1.27F22634020C8419596FCD6F5385025EDAD1EED4&ei=hUcBWpj6IJWKcO6hq2g&expire=1510054885&source=youtube&dur=1205.400&clen=89031435&initcwndbps=1397500&requiressl=yes&lmt=1509220818019876&ipbits=0&ratebypass=yes", "height": 480, "format_note": "480p", "format": "244 - 854x480 (480p)", "protocol": "https", "ext": "webm", "fps": 30, "format_id": "244", "player_url": null, "width": 854, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "vp9", "filesize": 89031435, "tbr": 763.921, "acodec": "none"}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?id=e48c10d001af7ee4&itag=135&source=youtube&requiressl=yes&mn=sn-4g5ednld&ms=au&mv=m&ei=hkcBWs3jEIvscp-5gagE&pl=17&mm=31&initcwndbps=1311250&ratebypass=yes&mime=video/mp4&gir=yes&clen=83197723&lmt=1510029660126496&dur=1205.399&key=dg_yt0&signature=36B57731F1A8AB2F55FFD8928440DDBED36FC3DB.969F35671363277D50A0E630032D1B3398F5A5A4&mt=1510033130&ip=149.172.215.29&ipbits=0&expire=1510054886&sparams=ip,ipbits,expire,id,itag,source,requiressl,mn,ms,mv,ei,pl,mm,initcwndbps,ratebypass,mime,gir,clen,lmt,dur", "protocol": "https", "format_note": "DASH video", "format": "135 - 854x480 (DASH video)", "ext": "mp4", "format_id": "135", "width": 854, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "height": 480, "fps": 30, "manifest_url": "https://manifest.googlevideo.com/api/manifest/dash/id/e48c10d001af7ee4/requiressl/yes/key/yt6/mn/sn-4g5ednld/expire/1510054886/sparams/as%2Cei%2Chfr%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Cplayback_host%2Crequiressl%2Csource%2Cexpire/source/youtube/itag/0/ms/au/playback_host/r4---sn-4g5ednld.googlevideo.com/mv/m/ei/hkcBWs3jEIvscp-5gagE/signature/E3C9CB7E639DAC1F379DD3286CFB5219BEC1DF56.16DAB28033C7AC96D8875B4DF9A3FBAEBB4E05D7/pl/17/ipbits/0/mm/31/hfr/1/initcwndbps/1311250/ip/149.172.215.29/mt/1510033130/as/fmp4_audio_clear%2Cfmp4_sd_hd_clear", "language": null, "vcodec": "avc1.4d401f", "filesize": 83197723, "tbr": 1190.586, "acodec": "none", "asr": null}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mn=sn-4g5ednld&mm=31&ip=149.172.215.29&gir=yes&pl=17&aitags=133%2C134%2C135%2C136%2C137%2C160%2C242%2C243%2C244%2C247%2C248%2C278%2C298%2C299%2C302%2C303&mime=video%2Fwebm&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&mv=m&sparams=aitags%2Cclen%2Cdur%2Cei%2Cgir%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Ckeepalive%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Crequiressl%2Csource%2Cexpire&mt=1510033130&itag=247&keepalive=yes&key=yt6&signature=5776A02A134302997FE0EF2712B76C7CEDFF64A2.DCD2B7B001B563E883C96118854AB93B664B7014&ei=hUcBWpj6IJWKcO6hq2g&expire=1510054885&source=youtube&dur=1205.400&clen=181684548&initcwndbps=1397500&requiressl=yes&lmt=1509220822466355&ipbits=0&ratebypass=yes", "height": 720, "format_note": "720p", "format": "247 - 1280x720 (720p)", "protocol": "https", "ext": "webm", "fps": 30, "format_id": "247", "player_url": null, "width": 1280, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "vp9", "filesize": 181684548, "tbr": 1518.869, "acodec": "none"}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mn=sn-4g5ednld&mm=31&ip=149.172.215.29&gir=yes&pl=17&aitags=133%2C134%2C135%2C136%2C137%2C160%2C242%2C243%2C244%2C247%2C248%2C278%2C298%2C299%2C302%2C303&mime=video%2Fmp4&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&mv=m&sparams=aitags%2Cclen%2Cdur%2Cei%2Cgir%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Ckeepalive%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Crequiressl%2Csource%2Cexpire&mt=1510033130&itag=136&keepalive=yes&key=yt6&signature=48CB5B7630E9B2EF064FEAA90283C93600C8C99A.97A6FF092B50C45B93A9D00A4489951CAC420264&ei=hUcBWpj6IJWKcO6hq2g&expire=1510054885&source=youtube&dur=1205.399&clen=159409255&initcwndbps=1397500&requiressl=yes&lmt=1510029660038529&ipbits=0&ratebypass=yes", "height": 720, "format_note": "720p", "format": "136 - 1280x720 (720p)", "protocol": "https", "ext": "mp4", "fps": 30, "format_id": "136", "player_url": null, "width": 1280, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "avc1.4d401f", "filesize": 159409255, "tbr": 2426.878, "acodec": "none"}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mn=sn-4g5ednld&mm=31&ip=149.172.215.29&gir=yes&pl=17&aitags=133%2C134%2C135%2C136%2C137%2C160%2C242%2C243%2C244%2C247%2C248%2C278%2C298%2C299%2C302%2C303&mime=video%2Fwebm&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&mv=m&sparams=aitags%2Cclen%2Cdur%2Cei%2Cgir%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Ckeepalive%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Crequiressl%2Csource%2Cexpire&mt=1510033130&itag=302&keepalive=yes&key=yt6&signature=63C83F206363E38FCBEE6FD169C841E94D3F9E75.B5FA9E01794F479B511E3C00DED483C43A18079F&ei=hUcBWpj6IJWKcO6hq2g&expire=1510054885&source=youtube&dur=1205.383&clen=287851935&initcwndbps=1397500&requiressl=yes&lmt=1509220334930529&ipbits=0&ratebypass=yes", "filesize": 287851935, "protocol": "https", "format_note": "720p60", "format": "302 - 1280x720 (720p60)", "ext": "webm", "fps": 60, "format_id": "302", "player_url": null, "width": 1280, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "vp9", "height": 720, "tbr": 2655.856, "acodec": "none"}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mn=sn-4g5ednld&mm=31&ip=149.172.215.29&gir=yes&pl=17&aitags=133%2C134%2C135%2C136%2C137%2C160%2C242%2C243%2C244%2C247%2C248%2C278%2C298%2C299%2C302%2C303&mime=video%2Fwebm&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&mv=m&sparams=aitags%2Cclen%2Cdur%2Cei%2Cgir%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Ckeepalive%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Crequiressl%2Csource%2Cexpire&mt=1510033130&itag=248&keepalive=yes&key=yt6&signature=4DA17D83E791B0C197A2D3967D7A75F59CE8F25A.2309F16DF7609A5254AAA09E1E8233DB1D25C3C8&ei=hUcBWpj6IJWKcO6hq2g&expire=1510054885&source=youtube&dur=1205.400&clen=333440838&initcwndbps=1397500&requiressl=yes&lmt=1509220321093101&ipbits=0&ratebypass=yes", "height": 1080, "format_note": "1080p", "format": "248 - 1920x1080 (1080p)", "protocol": "https", "ext": "webm", "fps": 30, "format_id": "248", "player_url": null, "width": 1920, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "vp9", "filesize": 333440838, "tbr": 2688.337, "acodec": "none"}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?id=e48c10d001af7ee4&itag=298&source=youtube&requiressl=yes&mn=sn-4g5ednld&ms=au&mv=m&ei=hkcBWs3jEIvscp-5gagE&pl=17&mm=31&initcwndbps=1311250&ratebypass=yes&mime=video/mp4&gir=yes&clen=259181823&lmt=1510029638957866&dur=1205.383&key=dg_yt0&signature=7B07F1D7FFAB14B4EDC22B12C7F5D83CD1491062.5CDB814AB2D30C985AF2DB6773CE9E7875EDD0AE&mt=1510033130&ip=149.172.215.29&ipbits=0&expire=1510054886&sparams=ip,ipbits,expire,id,itag,source,requiressl,mn,ms,mv,ei,pl,mm,initcwndbps,ratebypass,mime,gir,clen,lmt,dur", "protocol": "https", "format_note": "DASH video", "format": "298 - 1280x720 (DASH video)", "ext": "mp4", "format_id": "298", "width": 1280, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "height": 720, "fps": 60, "manifest_url": "https://manifest.googlevideo.com/api/manifest/dash/id/e48c10d001af7ee4/requiressl/yes/key/yt6/mn/sn-4g5ednld/expire/1510054886/sparams/as%2Cei%2Chfr%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Cplayback_host%2Crequiressl%2Csource%2Cexpire/source/youtube/itag/0/ms/au/playback_host/r4---sn-4g5ednld.googlevideo.com/mv/m/ei/hkcBWs3jEIvscp-5gagE/signature/E3C9CB7E639DAC1F379DD3286CFB5219BEC1DF56.16DAB28033C7AC96D8875B4DF9A3FBAEBB4E05D7/pl/17/ipbits/0/mm/31/hfr/1/initcwndbps/1311250/ip/149.172.215.29/mt/1510033130/as/fmp4_audio_clear%2Cfmp4_sd_hd_clear", "language": null, "vcodec": "avc1.4d4020", "filesize": 259181823, "tbr": 3464.832, "acodec": "none", "asr": null}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mn=sn-4g5ednld&mm=31&ip=149.172.215.29&gir=yes&pl=17&aitags=133%2C134%2C135%2C136%2C137%2C160%2C242%2C243%2C244%2C247%2C248%2C278%2C298%2C299%2C302%2C303&mime=video%2Fmp4&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&mv=m&sparams=aitags%2Cclen%2Cdur%2Cei%2Cgir%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Ckeepalive%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Crequiressl%2Csource%2Cexpire&mt=1510033130&itag=137&keepalive=yes&key=yt6&signature=28189F4C643C09EA708112A0389C37A5F8847C8A.B9487C1C17657C09E51D914ADDA8D58D1D54E31F&ei=hUcBWpj6IJWKcO6hq2g&expire=1510054885&source=youtube&dur=1205.399&clen=293453011&initcwndbps=1397500&requiressl=yes&lmt=1510029643904414&ipbits=0&ratebypass=yes", "height": 1080, "format_note": "1080p", "format": "137 - 1920x1080 (1080p)", "protocol": "https", "ext": "mp4", "fps": 30, "format_id": "137", "player_url": null, "width": 1920, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "avc1.640028", "filesize": 293453011, "tbr": 4322.702, "acodec": "none"}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mn=sn-4g5ednld&mm=31&ip=149.172.215.29&gir=yes&pl=17&aitags=133%2C134%2C135%2C136%2C137%2C160%2C242%2C243%2C244%2C247%2C248%2C278%2C298%2C299%2C302%2C303&mime=video%2Fwebm&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&mv=m&sparams=aitags%2Cclen%2Cdur%2Cei%2Cgir%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Ckeepalive%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Crequiressl%2Csource%2Cexpire&mt=1510033130&itag=303&keepalive=yes&key=yt6&signature=62EF76A63A335734356F809480D0A388380C8E69.765AE7BD2F90530A4E906B7CABF4A28C207C84A1&ei=hUcBWpj6IJWKcO6hq2g&expire=1510054885&source=youtube&dur=1205.383&clen=542453753&initcwndbps=1397500&requiressl=yes&lmt=1509220755544739&ipbits=0&ratebypass=yes", "filesize": 542453753, "protocol": "https", "format_note": "1080p60", "format": "303 - 1920x1080 (1080p60)", "ext": "webm", "fps": 60, "format_id": "303", "player_url": null, "width": 1920, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "vp9", "height": 1080, "tbr": 4442.198, "acodec": "none"}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?id=e48c10d001af7ee4&itag=299&source=youtube&requiressl=yes&mn=sn-4g5ednld&ms=au&mv=m&ei=hkcBWs3jEIvscp-5gagE&pl=17&mm=31&initcwndbps=1311250&ratebypass=yes&mime=video/mp4&gir=yes&clen=515535482&lmt=1510029653727916&dur=1205.383&key=dg_yt0&signature=1702D702941E3F03DBBA58CDF9E9F59A51031584.2B1D90096C036ECF749B843F65F4DEF4939F0667&mt=1510033130&ip=149.172.215.29&ipbits=0&expire=1510054886&sparams=ip,ipbits,expire,id,itag,source,requiressl,mn,ms,mv,ei,pl,mm,initcwndbps,ratebypass,mime,gir,clen,lmt,dur", "protocol": "https", "format_note": "DASH video", "format": "299 - 1920x1080 (DASH video)", "ext": "mp4", "format_id": "299", "width": 1920, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "height": 1080, "fps": 60, "manifest_url": "https://manifest.googlevideo.com/api/manifest/dash/id/e48c10d001af7ee4/requiressl/yes/key/yt6/mn/sn-4g5ednld/expire/1510054886/sparams/as%2Cei%2Chfr%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Cplayback_host%2Crequiressl%2Csource%2Cexpire/source/youtube/itag/0/ms/au/playback_host/r4---sn-4g5ednld.googlevideo.com/mv/m/ei/hkcBWs3jEIvscp-5gagE/signature/E3C9CB7E639DAC1F379DD3286CFB5219BEC1DF56.16DAB28033C7AC96D8875B4DF9A3FBAEBB4E05D7/pl/17/ipbits/0/mm/31/hfr/1/initcwndbps/1311250/ip/149.172.215.29/mt/1510033130/as/fmp4_audio_clear%2Cfmp4_sd_hd_clear", "language": null, "vcodec": "avc1.64002a", "filesize": 515535482, "tbr": 6644.16, "acodec": "none", "asr": null}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mn=sn-4g5ednld&mm=31&ip=149.172.215.29&gir=yes&pl=17&mime=video%2F3gpp&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&mv=m&sparams=clen%2Cdur%2Cei%2Cgir%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Crequiressl%2Csource%2Cexpire&mt=1510033130&itag=17&key=yt6&signature=C968E1FB66D8DDAC93765DD3B1812379EE27D178.32BC520DD34447841F407AA82AF801E9866BEE4F&ei=hUcBWpj6IJWKcO6hq2g&expire=1510054885&source=youtube&dur=1205.487&clen=12043617&initcwndbps=1397500&requiressl=yes&lmt=1510026573930720&ipbits=0&ratebypass=yes", "protocol": "https", "format_note": "small", "format": "17 - 176x144 (small)", "resolution": "176x144", "ext": "3gp", "format_id": "17", "player_url": null, "width": 176, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "mp4v.20.3", "height": 144, "acodec": "mp4a.40.2", "abr": 24}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mn=sn-4g5ednld&mm=31&ip=149.172.215.29&gir=yes&pl=17&mime=video%2F3gpp&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&mv=m&sparams=clen%2Cdur%2Cei%2Cgir%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Crequiressl%2Csource%2Cexpire&mt=1510033130&itag=36&key=yt6&signature=5712D902EDB358A4A6E6DEE63A8E75B98DE043F1.D201F67D0879E6385B4829F7FB92A11D3C0ADC46&ei=hUcBWpj6IJWKcO6hq2g&expire=1510054885&source=youtube&dur=1205.487&clen=35676271&initcwndbps=1397500&requiressl=yes&lmt=1510026579839939&ipbits=0&ratebypass=yes", "protocol": "https", "format_note": "small", "format": "36 - 320x180 (small)", "resolution": "320x180", "ext": "3gp", "format_id": "36", "player_url": null, "width": 320, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "mp4v.20.3", "height": 180, "acodec": "mp4a.40.2"}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mn=sn-4g5ednld&mm=31&ip=149.172.215.29&gir=yes&pl=17&mime=video%2Fwebm&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&mv=m&sparams=clen%2Cdur%2Cei%2Cgir%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Cratebypass%2Crequiressl%2Csource%2Cexpire&mt=1510033130&itag=43&key=yt6&signature=31B81408DCF637DCF2C7FFA7F2E78BD19A8FFB2A.BB5B211BB5ACA526458943F34EB4EA1BB3A9EDB4&ei=hUcBWpj6IJWKcO6hq2g&expire=1510054885&source=youtube&dur=0.000&clen=99457955&initcwndbps=1397500&requiressl=yes&ratebypass=yes&lmt=1510032726201608&ipbits=0", "protocol": "https", "format_note": "medium", "format": "43 - 640x360 (medium)", "resolution": "640x360", "ext": "webm", "format_id": "43", "player_url": null, "width": 640, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "vp8.0", "height": 360, "acodec": "vorbis", "abr": 128}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mn=sn-4g5ednld&mm=31&ip=149.172.215.29&gir=yes&pl=17&mime=video%2Fmp4&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&mv=m&sparams=clen%2Cdur%2Cei%2Cgir%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Cratebypass%2Crequiressl%2Csource%2Cexpire&mt=1510033130&itag=18&key=yt6&signature=6D95A7B871CEE1D3754C749E010FA773BED56DB4.ABB9AF86C5DD9BC294881108C6F4B43973A0AEF6&ei=hUcBWpj6IJWKcO6hq2g&expire=1510054885&source=youtube&dur=1205.440&clen=90380839&initcwndbps=1397500&requiressl=yes&ratebypass=yes&lmt=1510029875775472&ipbits=0", "protocol": "https", "format_note": "medium", "format": "18 - 640x360 (medium)", "resolution": "640x360", "ext": "mp4", "format_id": "18", "player_url": null, "width": 640, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "avc1.42001E", "height": 360, "acodec": "mp4a.40.2", "abr": 96}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mt=1510033130&mn=sn-4g5ednld&mm=31&ip=149.172.215.29&pl=17&mime=video%2Fmp4&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&initcwndbps=1397500&mv=m&sparams=dur%2Cei%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Cratebypass%2Crequiressl%2Csource%2Cexpire&requiressl=yes&itag=22&ratebypass=yes&lmt=1510030215409299&key=yt6&signature=08ECBD1DE74B8ADD925F79696769C76D29C4AB71.6999C2F92DAD11E994B79D1914B4F543BA8C2A0B&ei=hUcBWpj6IJWKcO6hq2g&dur=1205.440&expire=1510054885&source=youtube&ipbits=0", "protocol": "https", "format_note": "hd720", "format": "22 - 1280x720 (hd720)", "resolution": "1280x720", "ext": "mp4", "format_id": "22", "player_url": null, "width": 1280, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "avc1.64001F", "height": 720, "acodec": "mp4a.40.2", "abr": 192}], "chapters": null, "_filename": "StarCraft - Remastered - UNITED EARTH DIRECTORATE!-5IwQ0AGvfuQ.mp4", "tags": [], "duration": 1205, "id": "5IwQ0AGvfuQ", "average_rating": 4.95571947098, "format_id": "299+251", "resolution": null, "vcodec": "avc1.64002a", "age_limit": 0, "creator": null, "fps": 60, "automatic_captions": {}, "playlist": null, "vbr": null, "uploader": "LowkoTV", "license": null, "display_id": "5IwQ0AGvfuQ", "view_count": 19264, "stretched_ratio": null, "playlist_index": null, "abr": 160, "annotations": null}

    VMVodData& vodData = downLoadData._vod.data();
    VMVodDescriptionData& descData = vodData.description.data();
    QJsonObject root = doc.object();
    descData._webPageUrl = root.value(QStringLiteral("webpage_url")).toString();
    if (descData._webPageUrl.isEmpty()) {
        QString message = QStringLiteral("Invalid youtube-dl JSON response: ") + output.data();
        qCritical() << message;
        downLoadData.error = VMVodEnums::VM_ErrorInvalidResponse;
        downLoadData.errorMessage = message;
        emit vodMetaDataDownloadCompleted(id, download);
        return;
    }

    descData._fullTitle = root.value(QStringLiteral("fulltitle")).toString();
    descData._title = root.value(QStringLiteral("title")).toString();
    descData._id = root.value(QStringLiteral("id")).toString();

    descData.durationS = root.value("duration").toInt();
    descData._thumbnail = root.value(QStringLiteral("thumbnail")).toString();

    QJsonArray formats = root.value(QStringLiteral("formats")).toArray();
    int lastResortIndex = -1;
    for (int i = 0; i < formats.size(); ++i) {
        QJsonObject format = formats[i].toObject();
        QString vcodec = format.value(QStringLiteral("vcodec")).toString();
        QString acodec = format.value(QStringLiteral("acodec")).toString();

        auto hasVideo = !vcodec.isEmpty() && QStringLiteral("none") != vcodec;
        auto hasAudio = !acodec.isEmpty() && QStringLiteral("none") != acodec;

        if (hasVideo) {
            if (lastResortIndex == -1) {
                lastResortIndex = i;
            }

            if (hasAudio) {
                //"format_id": "160p30",
                vodData._formats.append(VMVodFormat());
                VMVodFormat& vodFormat = vodData._formats.last();
                VMVodFormatData& vodFormatData = vodFormat.data();
                vodFormatData._vodUrl = descData._webPageUrl;
                vodFormatData._width = format.value(QStringLiteral("width")).toInt();
                vodFormatData._height = format.value(QStringLiteral("height")).toInt();
                vodFormatData._id = format.value(QStringLiteral("format_id")).toString();
                vodFormatData._fileUrl = format.value(QStringLiteral("url")).toString();
                vodFormatData._displayName = format.value(QStringLiteral("format")).toString();
                vodFormatData._fileExtension = format.value(QStringLiteral("ext")).toString();
                fillFrameRate(vodFormat, format);
                fillFormatId(vodFormat);
                //        qDebug() << vodFormatData._width << vodFormatData._height << vodFormatData._format;
            }
        }
    }

    if (vodData._formats.isEmpty()) {
        if (formats.isEmpty()) {
            QString message = QStringLiteral("No format has video");
            qCritical() << message;
            downLoadData.error = VMVodEnums::VM_ErrorNoVideo;
            downLoadData.errorMessage = message;
            emit vodMetaDataDownloadCompleted(id, download);
        } else {
            if (lastResortIndex == -1) {
                lastResortIndex = 0;
            }

            QJsonObject format = formats[lastResortIndex].toObject();
            vodData._formats.append(VMVodFormat());
            VMVodFormat& vodFormat = vodData._formats.last();
            VMVodFormatData& vodFormatData = vodFormat.data();
            vodFormatData._vodUrl = descData._webPageUrl;
            vodFormatData._width = format.value(QStringLiteral("width")).toInt();
            vodFormatData._height = format.value(QStringLiteral("height")).toInt();
            vodFormatData._id = format.value(QStringLiteral("format_id")).toString();
            vodFormatData._fileUrl = format.value(QStringLiteral("url")).toString();
            vodFormatData._displayName = format.value(QStringLiteral("format")).toString();
            vodFormatData._fileExtension = format.value(QStringLiteral("ext")).toString();
            fillFrameRate(vodFormat, format);
            fillFormatId(vodFormat);
        }
    }

    auto cacheEntry = new CacheEntry;
    cacheEntry->fetchTime = QDateTime::currentDateTime();
    cacheEntry->vod = downLoadData._vod;
    m_MetaDataCache.insert(downLoadData._url, cacheEntry);

    qDebug() << "emit fetchVodMetaDataCompleted("<< id << ", " << download << ")";
    emit vodMetaDataDownloadCompleted(id, download);
}

void
VMYTDL::onVodFileProcessFinished(int code, QProcess::ExitStatus status)
{
    QProcess* process = qobject_cast<QProcess*>(QObject::sender());
    Q_ASSERT(process);

    cleanupProcess(process);

    qDebug() << "youtube-dl vod file process pid" << process->pid()
             << "finished, status:" << status
             << ", exit code:" << code;

    auto result = m_ProcessMap.value(process);
    m_ProcessMap.remove(process);
    const auto id = qvariant_cast<qint64>(result[s_Token]);

    VMVodFileDownload download = qvariant_cast<VMVodFileDownload>(result[s_Download]);
    VMVodFileDownloadData& downLoadData = download.data();

    auto processError = QString::fromLocal8Bit(process->readAllStandardError());
    auto errorLines = processError.splitRef(QChar('\n'), QString::SkipEmptyParts);
    for (int i = 0; i < errorLines.size(); ++i) {
        const auto& line = errorLines[i];
        qDebug() << "stderr" << line;
        if (line.indexOf(QStringLiteral("ERROR:")) == 0) {
            if (line.indexOf(QStringLiteral("[Errno -2]"), 0, Qt::CaseInsensitive) >= 0) {
                // ERROR: unable to download video data: <urlopen error [Errno -2] Name or service not known>
                download.data().error = VMVodEnums::VM_ErrorNetworkDown;
            } else if (line.indexOf(QStringLiteral("[Errno 28]"), 0, Qt::CaseInsensitive) >= 0) {
                // ERROR: unable to write data: [Errno 28] No space left on device
                download.data().error = VMVodEnums::VM_ErrorNoSpaceLeftOnDevice;
            } else if (line.indexOf(QStringLiteral("timed out"), 0, Qt::CaseInsensitive) >= 0) {
                // stderr "ERROR: unable to download video data: <urlopen error _ssl.c:584: The handshake operation timed out>"
                download.data().error = VMVodEnums::VM_ErrorTimedOut;
            } else {
                download.data().error = VMVodEnums::VM_ErrorUnknown;
            }
            downLoadData.errorMessage = line.toString();
            emit vodFileDownloadCompleted(id, download);
            return;
        }
    }

    download.data()._progress = 1;

    qDebug() << id << download;
    emit vodFileDownloadCompleted(id, download);
}

void
VMYTDL::onProcessError(QProcess::ProcessError error)
{
    QProcess* process = qobject_cast<QProcess*>(QObject::sender());
    Q_ASSERT(process);

    qDebug() << "process pid " << process->pid() << "error" << error;

    cleanupProcess(process);


    QVariantMap result = m_ProcessMap.value(process);
    m_ProcessMap.remove(process);
    qint64 id = qvariant_cast<qint64>(result[s_Token]);
    m_VodDownloads.remove(id);


    if (result[s_Type] == s_MetaData) {
        auto download = qvariant_cast<VMVodMetaDataDownload>(result[s_Download]);
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
        emit vodMetaDataDownloadCompleted(id, download);
    } else {
        auto download = qvariant_cast<VMVodFileDownload>(result[s_Download]);
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
        emit vodFileDownloadCompleted(id, download);
    }
}

void
VMYTDL::onYoutubeDlVodFileDownloadProcessReadReady()
{
    QProcess* process = qobject_cast<QProcess*>(QObject::sender());
    Q_ASSERT(process);

    qDebug() << "process pid" << process->pid();


    if (!m_ProcessMap.contains(process)) {
        qDebug() << "process pid" << process->pid() << "gone";
        return;
    }

    const QVariantMap result = m_ProcessMap.value(process);
    qint64 id = qvariant_cast<qint64>(result[s_Token]);
    VMVodFileDownload download = qvariant_cast<VMVodFileDownload>(result[s_Download]);
    float progress = -1;

    QByteArray data = process->readAll();
    int start = 0, end = 0;
    while (end < data.size()) {
        if (data[end] == '\r' || data[end] == '\n') {
            if (start < end) {
                QString str = QString::fromLocal8Bit(data.data() + start, end - start);
//                qDebug() << "line" << str;
                if (s_YTDLProgressRegexp.indexIn(str) != -1) {
                    QString capture = s_YTDLProgressRegexp.cap(1);
                    bool ok = false;
                    auto value = capture.toFloat(&ok);
                    if (ok) {
//                        qDebug() << "progress" << value;
                        progress = value;
                    }
                }
            }

            start = ++end;
        } else {
            ++end;
        }
    }

    if (start < end) {
        QString str = QString::fromLocal8Bit(data.data() + start, end - start);
        if (s_YTDLProgressRegexp.indexIn(str) != -1) {
            QString capture = s_YTDLProgressRegexp.cap(1);
            bool ok = false;
            auto value = capture.toFloat(&ok);
            if (ok) {
                progress = value;
            }
        }
    }

    if (progress >= 0) {
        download.data()._progress = qMin<float>(progress/100, 1);
        qDebug() << "process pid" << process->pid() << "progress" << download.data()._progress;
    }

    // update file size
    QFileInfo fi(download.data()._filePath);
    if (fi.exists()) {
        download.data().fileSize = fi.size();
    } else {
        download.data().fileSize = 0;
    }
    // update changed
    download.data().timeChanged = QDateTime::currentDateTime();
    // go go go
    emit vodFileDownloadChanged(id, download);
}

void
VMYTDL::cleanupProcess(QProcess* process) {
    Q_ASSERT(process);
    process->disconnect();
    process->deleteLater();
}

QVariantList
VMYTDL::inProgressVodFetches() {

    QVariantList result;
    result.reserve(m_VodDownloads.size());
    foreach(QProcess* process, m_VodDownloads.values()) {
        result << m_ProcessMap[process];
    }

    return result;
}

void
VMYTDL::fillFrameRate(VMVodFormat& format, const QJsonObject& json) const {
    int frameRate = -1;
    auto jsonFps = json.value(QStringLiteral("fps")).toString();
    if (jsonFps.isEmpty()) {
        if (s_YTDLProgressRegexp.indexIn(format.id()) != -1) {
            auto cap = s_YTDLProgressRegexp.cap(1);
            frameRate = cap.toInt();
        }
    } else {
        frameRate = jsonFps.toInt();
    }

    format.data()._frameRate = frameRate;
}

void
VMYTDL::fillFormatId(VMVodFormat& f) const
{
    VMVodEnums::Format format = VMVodEnums::VM_Unknown;

    auto value = f.height();
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

    f.data()._format = format;
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
    env.insert(QStringLiteral("LC_ALL"), QStringLiteral("C"));
    process->setProcessEnvironment(env);
    return process;
}
