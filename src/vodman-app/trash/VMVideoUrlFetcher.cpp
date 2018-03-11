/*-
 * Copyright (c) 2015 Peter Tworek, 2017 Jean Gressmann
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of any co-contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "VMVideoUrlFetcher.h"
#include "VMVod.h"

#include <QJsonDocument>
#include <sailfishapp.h>
#include <QJsonObject>
#include <QStringList>
#include <QProcess>
#include <QDebug>
#include <QDir>
#include <QProcess>
#include <QThread>
#include <QRegExp>



namespace {
const char kYouTubeDLBinaryName[] = "youtube-dl";
const int kMaxResponseCacheSize = 20;
const QRegExp s_ProgressRegexp("\\[download\\]\\s+(\\d+\\.\\d*)%\\s+");

QString getYouTubeDLPath()
{
    static QString program;
    if (program.isEmpty()) {
        program = SailfishApp::pathTo("bin").toLocalFile();
        program.append(QDir::separator());
        program.append(kYouTubeDLBinaryName);
        Q_ASSERT(QFile(program).exists());
    }
    return program;
}



} // anon

//QCache<QString, QVariantMap> VMVideoUrlFetcher::_response_cache;
QString VMVideoUrlFetcher::_version_str;
bool VMVideoUrlFetcher::_works = true;


VMVideoUrlFetcher::~VMVideoUrlFetcher() {

//    for (int i = 0; i < m_Watcher.size(); ++i) {
//        QFutureWatcher<QVariantMap>& watcher = m_Watcher[i];
//        if (watcher.isFinished()) {
//            continue;
//        }

//        watcher.cancel();
//        watcher.waitForFinished();
//    }


//    QMutexLocker g(&m_Lock);
//    ProcessMap::const_iterator it = m_ProcessMap.cbegin();
//    ProcessMap::const_iterator end = m_ProcessMap.cend();
//    for (; it != end; ++it) {
//        cleanupProcess(it.key());
//        it.key()->kill();
//    }

//    m_ProcessMap.clear();
}

VMVideoUrlFetcher::VMVideoUrlFetcher(QObject* parent)
    : QObject(parent)
//    , _process(0)
{
}

void
VMVideoUrlFetcher::runInitialCheck()
{
    QStringList arguments;
    arguments << "--version";

    qDebug() << "Attempting to start" << getYouTubeDLPath();

    QProcess process;
    process.start(getYouTubeDLPath(), arguments, QIODevice::ReadOnly);
    process.waitForFinished();

    if (process.exitStatus() == QProcess::NormalExit &&
        process.exitCode() == 0) {
        _version_str = process.readAllStandardOutput();
        _version_str = _version_str.simplified();
        _works = true;
        qDebug() << "youtube-dl works, current version:" << _version_str;
    } else {
        qCritical() << "youtune-dl is non functional:" << process.readAllStandardError();
        _works = false;
    }
}

void
VMVideoUrlFetcher::fetchMetaData(int id, QUrl url)
{
    QVariantMap result;
    result["type"] = QStringLiteral("metadata");
    result["request_url"] = url;
    result["id"] = id;

    if (!_works) {
        // fix me
        result["message"] = QStringLiteral("no working youtube-dl @ %1").arg(getYouTubeDLPath());
        emit metaDataFetchComplete(id, result);
        return;
    }

    qDebug() << "Trying to obtain video urls for: " << url;

    QMutexLocker g(&m_Lock);
    if (_response_cache.contains(url)) {
        qDebug() << "Response for" << url << "available in cache, using it";
        result = *_response_cache[url];
        result["id"] = id;
        emit metaDataFetchComplete(id, result);
        return;
    }

    QStringList arguments;
    arguments << "--dump-json"
              << "--youtube-skip-dash-manifest"
              << "--no-cache-dir"
              << "--no-call-home"
              << url.toString();

    qDebug() << "YouTubeDL subprocess:" << getYouTubeDLPath() << arguments;

    QProcess* process = new QProcess(this);
    connect(process, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(onMetaDataProcessFinished(int, QProcess::ExitStatus)));
    connect(process, SIGNAL(error(QProcess::ProcessError)),
            this, SLOT(onProcessError(QProcess::ProcessError)));

    _processDataMap.insert(process, result);

    process->start(getYouTubeDLPath(), arguments, QIODevice::ReadOnly);


}


void
VMVideoUrlFetcher::fetchVod(int id, QUrl url, QString format, QString filePath)
{
    QVariantMap result;
    result["type"] = QStringLiteral("vod");
    result["format"] = format;
    result["file_path"] = filePath;
    result["request_url"] = url;
    result["id"] = id;
    result["progress"] = 0.f;

    if (!_works) {
        // fix me
        result["message"] = QStringLiteral("no working youtube-dl @ %1").arg(getYouTubeDLPath());
        emit vodStatusChanged(id, result);
        return;
    }

    qDebug() << "Trying to obtain video file for: " << url;

    // youtube-dl -f 160p30 https://www.twitch.tv/videos/161472611?t=07h49m09s --no-cache-dir --no-call-home --newline -o foo.bar

    QStringList arguments;
    arguments << "--newline"
              << "--no-part"
              << "--no-cache-dir"
              << "--no-call-home"
              << "-f" << format
              << "-o" << filePath
              << url.toString();

    qDebug() << "YouTubeDL subprocess:" << getYouTubeDLPath() << arguments;



    QProcess* process = new QProcess(this);
    QList<QMetaObject::Connection>& connections = _connections[process];
    connections << connect(process, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(onMetaDataProcessFinished(int,QProcess::ExitStatus)));
    connections << connect(process, &QProcess::readyReadStandardOutput,
            this, &VMVideoUrlFetcher::onVodProcessReadReady);
    connections << connect(process, SIGNAL(errorOccurred(QProcess::ProcessError)),
            this, SLOT(onProcessError(QProcess::ProcessError)));

    QMutexLocker g(&m_Lock);
    _processDataMap.insert(process, result);
    _vodDownloads.insert(id, process);

    qDebug() << "process ptr" << (void*)process;

    process->start(getYouTubeDLPath(), arguments, QIODevice::ReadOnly);
}

void
VMVideoUrlFetcher::cancelFetchVod(int id) {
    QMutexLocker g(&m_Lock);
    QProcess* process = _vodDownloads.value(id);
    if (process) {
        qDebug() << "kill" << process->processId();
        process->kill();
    }
}

void
VMVideoUrlFetcher::onMetaDataProcessFinished(int code, QProcess::ExitStatus status)
{
    QProcess* process = qobject_cast<QProcess*>(QObject::sender());
    Q_ASSERT(process);

    qDebug() << "process ptr" << (void*)process;

    cleanupProcess(process);

    qDebug() << "youtube-dl process finished, status:" << status
             << ", exit code:" << code;



    QVariantMap result;
    {
        QMutexLocker g(&m_Lock);
        result = _processDataMap.value(process);
        _processDataMap.remove(process);
    }

    result["exit_status"] = status;
    result["exit_code"] = code;
    const int id = result["id"].toInt();

    if (status != QProcess::NormalExit || code != 0) {
        QByteArray error = process->readAllStandardError();
        QString message = QStringLiteral("YouTubeDL process did not finish cleanly: ") + error;
        qCritical() << message;
        result["message"] = message;
        emit metaDataFetchComplete(id, result);
        return;
    }

    QByteArray output = process->readAllStandardOutput();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(output, &error);
    if (error.error != QJsonParseError::NoError) {
        qCritical() << "JSON parse error:" << error.errorString();
        result["message"] = QStringLiteral("JSON parse error: %1").arg(error.errorString());
        emit metaDataFetchComplete(id, result);
        return;
    }

    result["json"] = doc.toJson(QJsonDocument::Compact);


//    // twitch {"view_count": 19484, "width": 1920, "format_id": "Source", "display_id": "v161472611", "requested_subtitles": null, "fulltitle": "HSC XV Day 4", "extractor": "twitch:vod", "protocol": "m3u8_native", "timestamp": 1500814982, "subtitles": {"rechat": [{"ext": "json", "url": "https://rechat.twitch.tv/rechat-messages?video_id=v161472611&start=1500814982"}]}, "thumbnail": "https://static-cdn.jtvnw.net/s3_vods/a490bf1a61acbc89fee6_taketv_25826129648_682803294//thumb/thumb161472611-320x240.jpg", "formats": [{"manifest_url": "https://usher.ttvnw.net/vod/161472611?allow_audio_only=true&nauth=%7B%22user_id%22%3Anull%2C%22vod_id%22%3A161472611%2C%22expires%22%3A1510105630%2C%22chansub%22%3A%7B%22restricted_bitrates%22%3A%5B%5D%7D%2C%22privileged%22%3Afalse%2C%22https_required%22%3Afalse%7D&player=twitchweb&allow_source=true&allow_spectre=true&nauthsig=cd9650b6faa761e34c77a49b777e4fde6479be58", "ext": "mp4", "format": "Audio_Only - audio only", "tbr": 218.991, "protocol": "m3u8_native", "preference": null, "fps": null, "format_id": "Audio_Only", "acodec": "mp4a.40.2", "vcodec": "none", "http_headers": {"Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Encoding": "gzip, deflate", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)"}, "url": "https://fastly.vod.hls.ttvnw.net/a490bf1a61acbc89fee6_taketv_25826129648_682803294/audio_only/highlight-161472611-muted-F7A9TIGDWS.m3u8"}, {"width": 284, "manifest_url": "https://usher.ttvnw.net/vod/161472611?allow_audio_only=true&nauth=%7B%22user_id%22%3Anull%2C%22vod_id%22%3A161472611%2C%22expires%22%3A1510105630%2C%22chansub%22%3A%7B%22restricted_bitrates%22%3A%5B%5D%7D%2C%22privileged%22%3Afalse%2C%22https_required%22%3Afalse%7D&player=twitchweb&allow_source=true&allow_spectre=true&nauthsig=cd9650b6faa761e34c77a49b777e4fde6479be58", "ext": "mp4", "format": "160p30 - 284x160", "tbr": 228.015, "protocol": "m3u8_native", "preference": null, "fps": null, "format_id": "160p30", "acodec": "mp4a.40.2", "vcodec": "avc1.4D400C", "http_headers": {"Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Encoding": "gzip, deflate", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)"}, "url": "https://fastly.vod.hls.ttvnw.net/a490bf1a61acbc89fee6_taketv_25826129648_682803294/160p30/highlight-161472611-muted-F7A9TIGDWS.m3u8", "height": 160}, {"width": 640, "manifest_url": "https://usher.ttvnw.net/vod/161472611?allow_audio_only=true&nauth=%7B%22user_id%22%3Anull%2C%22vod_id%22%3A161472611%2C%22expires%22%3A1510105630%2C%22chansub%22%3A%7B%22restricted_bitrates%22%3A%5B%5D%7D%2C%22privileged%22%3Afalse%2C%22https_required%22%3Afalse%7D&player=twitchweb&allow_source=true&allow_spectre=true&nauthsig=cd9650b6faa761e34c77a49b777e4fde6479be58", "ext": "mp4", "format": "360p30 - 640x360", "tbr": 562.609, "protocol": "m3u8_native", "preference": null, "fps": null, "format_id": "360p30", "acodec": "mp4a.40.2", "vcodec": "avc1.4D401E", "http_headers": {"Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Encoding": "gzip, deflate", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)"}, "url": "https://fastly.vod.hls.ttvnw.net/a490bf1a61acbc89fee6_taketv_25826129648_682803294/360p30/highlight-161472611-muted-F7A9TIGDWS.m3u8", "height": 360}, {"width": 852, "manifest_url": "https://usher.ttvnw.net/vod/161472611?allow_audio_only=true&nauth=%7B%22user_id%22%3Anull%2C%22vod_id%22%3A161472611%2C%22expires%22%3A1510105630%2C%22chansub%22%3A%7B%22restricted_bitrates%22%3A%5B%5D%7D%2C%22privileged%22%3Afalse%2C%22https_required%22%3Afalse%7D&player=twitchweb&allow_source=true&allow_spectre=true&nauthsig=cd9650b6faa761e34c77a49b777e4fde6479be58", "ext": "mp4", "format": "480p30 - 852x480", "tbr": 1031.06, "protocol": "m3u8_native", "preference": null, "fps": null, "format_id": "480p30", "acodec": "mp4a.40.2", "vcodec": "avc1.4D401E", "http_headers": {"Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Encoding": "gzip, deflate", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)"}, "url": "https://fastly.vod.hls.ttvnw.net/a490bf1a61acbc89fee6_taketv_25826129648_682803294/480p30/highlight-161472611-muted-F7A9TIGDWS.m3u8", "height": 480}, {"width": 1280, "manifest_url": "https://usher.ttvnw.net/vod/161472611?allow_audio_only=true&nauth=%7B%22user_id%22%3Anull%2C%22vod_id%22%3A161472611%2C%22expires%22%3A1510105630%2C%22chansub%22%3A%7B%22restricted_bitrates%22%3A%5B%5D%7D%2C%22privileged%22%3Afalse%2C%22https_required%22%3Afalse%7D&player=twitchweb&allow_source=true&allow_spectre=true&nauthsig=cd9650b6faa761e34c77a49b777e4fde6479be58", "ext": "mp4", "format": "720p30 - 1280x720", "tbr": 1705.809, "protocol": "m3u8_native", "preference": null, "fps": null, "format_id": "720p30", "acodec": "mp4a.40.2", "vcodec": "avc1.4D401F", "http_headers": {"Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Encoding": "gzip, deflate", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)"}, "url": "https://fastly.vod.hls.ttvnw.net/a490bf1a61acbc89fee6_taketv_25826129648_682803294/720p30/highlight-161472611-muted-F7A9TIGDWS.m3u8", "height": 720}, {"width": 1920, "manifest_url": "https://usher.ttvnw.net/vod/161472611?allow_audio_only=true&nauth=%7B%22user_id%22%3Anull%2C%22vod_id%22%3A161472611%2C%22expires%22%3A1510105630%2C%22chansub%22%3A%7B%22restricted_bitrates%22%3A%5B%5D%7D%2C%22privileged%22%3Afalse%2C%22https_required%22%3Afalse%7D&player=twitchweb&allow_source=true&allow_spectre=true&nauthsig=cd9650b6faa761e34c77a49b777e4fde6479be58", "ext": "mp4", "format": "Source - 1920x1080", "tbr": 6325.641, "protocol": "m3u8_native", "preference": 10, "fps": null, "format_id": "Source", "acodec": "mp4a.40.2", "vcodec": "avc1.4D402A", "http_headers": {"Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Encoding": "gzip, deflate", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)"}, "url": "https://fastly.vod.hls.ttvnw.net/a490bf1a61acbc89fee6_taketv_25826129648_682803294/chunked/highlight-161472611-muted-F7A9TIGDWS.m3u8", "height": 1080}], "vcodec": "avc1.4D402A", "manifest_url": "https://usher.ttvnw.net/vod/161472611?allow_audio_only=true&nauth=%7B%22user_id%22%3Anull%2C%22vod_id%22%3A161472611%2C%22expires%22%3A1510105630%2C%22chansub%22%3A%7B%22restricted_bitrates%22%3A%5B%5D%7D%2C%22privileged%22%3Afalse%2C%22https_required%22%3Afalse%7D&player=twitchweb&allow_source=true&allow_spectre=true&nauthsig=cd9650b6faa761e34c77a49b777e4fde6479be58", "extractor_key": "TwitchVod", "description": "", "fps": null, "height": 1080, "_filename": "HSC XV Day 4-v161472611.mp4", "uploader_id": "taketv", "title": "HSC XV Day 4", "preference": 10, "uploader": "TaKeTV", "format": "Source - 1920x1080", "start_time": 28149.0, "playlist_index": null, "duration": 33528, "acodec": "mp4a.40.2", "webpage_url_basename": "161472611", "webpage_url": "https://www.twitch.tv/videos/161472611?t=07h49m09s", "ext": "mp4", "thumbnails": [{"url": "https://static-cdn.jtvnw.net/s3_vods/a490bf1a61acbc89fee6_taketv_25826129648_682803294//thumb/thumb161472611-320x240.jpg", "id": "0"}], "http_headers": {"Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Encoding": "gzip, deflate", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)"}, "id": "v161472611", "playlist": null, "tbr": 6325.641, "url": "https://fastly.vod.hls.ttvnw.net/a490bf1a61acbc89fee6_taketv_25826129648_682803294/chunked/highlight-161472611-muted-F7A9TIGDWS.m3u8", "upload_date": "20170723"}
//    // youtube {"uploader_url": null, "series": null, "width": 1920, "webpage_url": "https://www.youtube.com/watch?v=5IwQ0AGvfuQ", "format": "299 - 1920x1080 (DASH video)+251 - audio only (DASH audio)", "extractor_key": "Youtube", "end_time": null, "ext": "mp4", "subtitles": {}, "start_time": null, "thumbnail": "https://i.ytimg.com/vi/5IwQ0AGvfuQ/default.jpg", "description": "", "height": 1080, "title": "StarCraft: Remastered - UNITED EARTH DIRECTORATE!", "categories": null, "season_number": null, "requested_subtitles": null, "requested_formats": [{"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?id=e48c10d001af7ee4&itag=299&source=youtube&requiressl=yes&mn=sn-4g5ednld&ms=au&mv=m&ei=hkcBWs3jEIvscp-5gagE&pl=17&mm=31&initcwndbps=1311250&ratebypass=yes&mime=video/mp4&gir=yes&clen=515535482&lmt=1510029653727916&dur=1205.383&key=dg_yt0&signature=1702D702941E3F03DBBA58CDF9E9F59A51031584.2B1D90096C036ECF749B843F65F4DEF4939F0667&mt=1510033130&ip=149.172.215.29&ipbits=0&expire=1510054886&sparams=ip,ipbits,expire,id,itag,source,requiressl,mn,ms,mv,ei,pl,mm,initcwndbps,ratebypass,mime,gir,clen,lmt,dur", "manifest_url": "https://manifest.googlevideo.com/api/manifest/dash/id/e48c10d001af7ee4/requiressl/yes/key/yt6/mn/sn-4g5ednld/expire/1510054886/sparams/as%2Cei%2Chfr%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Cplayback_host%2Crequiressl%2Csource%2Cexpire/source/youtube/itag/0/ms/au/playback_host/r4---sn-4g5ednld.googlevideo.com/mv/m/ei/hkcBWs3jEIvscp-5gagE/signature/E3C9CB7E639DAC1F379DD3286CFB5219BEC1DF56.16DAB28033C7AC96D8875B4DF9A3FBAEBB4E05D7/pl/17/ipbits/0/mm/31/hfr/1/initcwndbps/1311250/ip/149.172.215.29/mt/1510033130/as/fmp4_audio_clear%2Cfmp4_sd_hd_clear", "height": 1080, "protocol": "https", "format_note": "DASH video", "format": "299 - 1920x1080 (DASH video)", "language": null, "ext": "mp4", "fps": 60, "format_id": "299", "width": 1920, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "avc1.64002a", "filesize": 515535482, "tbr": 6644.16, "acodec": "none", "asr": null}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mn=sn-4g5ednld&mm=31&ip=149.172.215.29&gir=yes&pl=17&mime=audio%2Fwebm&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&mv=m&sparams=clen%2Cdur%2Cei%2Cgir%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Ckeepalive%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Crequiressl%2Csource%2Cexpire&mt=1510033130&itag=251&keepalive=yes&key=yt6&signature=63C4DCAEE40A17284F3F531D872D25281C0E4A21.680B6BEB8931E76A38D27FED5EA749478FA35B34&ei=hUcBWpj6IJWKcO6hq2g&expire=1510054885&source=youtube&dur=1205.421&clen=19152907&initcwndbps=1397500&requiressl=yes&lmt=1509220063342381&ipbits=0&ratebypass=yes", "protocol": "https", "format_note": "DASH audio", "format": "251 - audio only (DASH audio)", "ext": "webm", "format_id": "251", "player_url": null, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "none", "filesize": 19152907, "tbr": 148.79, "acodec": "opus", "abr": 160}], "like_count": null, "fulltitle": "StarCraft: Remastered - UNITED EARTH DIRECTORATE!", "upload_date": null, "extractor": "youtube", "webpage_url_basename": "5IwQ0AGvfuQ", "is_live": null, "uploader_id": null, "episode_number": null, "thumbnails": [{"id": "0", "url": "https://i.ytimg.com/vi/5IwQ0AGvfuQ/default.jpg"}], "dislike_count": null, "acodec": "opus", "alt_title": null, "formats": [{"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?id=e48c10d001af7ee4&itag=139&source=youtube&requiressl=yes&mn=sn-4g5ednld&ms=au&mv=m&ei=hkcBWs3jEIvscp-5gagE&pl=17&mm=31&initcwndbps=1311250&ratebypass=yes&mime=audio/mp4&gir=yes&clen=7170087&lmt=1510029848295527&dur=1205.533&key=dg_yt0&signature=881BF3353FB0A0D2EA8BF7CECE15D900EC765801.0F52D0863D85E7DA66B17C00AAF628CDA498EEAF&mt=1510033130&ip=149.172.215.29&ipbits=0&expire=1510054886&sparams=ip,ipbits,expire,id,itag,source,requiressl,mn,ms,mv,ei,pl,mm,initcwndbps,ratebypass,mime,gir,clen,lmt,dur", "protocol": "https", "format_note": "DASH audio", "format": "139 - audio only (DASH audio)", "ext": "m4a", "format_id": "139", "width": null, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "height": null, "asr": 22050, "fps": null, "manifest_url": "https://manifest.googlevideo.com/api/manifest/dash/id/e48c10d001af7ee4/requiressl/yes/key/yt6/mn/sn-4g5ednld/expire/1510054886/sparams/as%2Cei%2Chfr%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Cplayback_host%2Crequiressl%2Csource%2Cexpire/source/youtube/itag/0/ms/au/playback_host/r4---sn-4g5ednld.googlevideo.com/mv/m/ei/hkcBWs3jEIvscp-5gagE/signature/E3C9CB7E639DAC1F379DD3286CFB5219BEC1DF56.16DAB28033C7AC96D8875B4DF9A3FBAEBB4E05D7/pl/17/ipbits/0/mm/31/hfr/1/initcwndbps/1311250/ip/149.172.215.29/mt/1510033130/as/fmp4_audio_clear%2Cfmp4_sd_hd_clear", "language": null, "container": "m4a_dash", "vcodec": "none", "filesize": 7170087, "tbr": 49.685, "acodec": "mp4a.40.5", "abr": 48}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mn=sn-4g5ednld&mm=31&ip=149.172.215.29&gir=yes&pl=17&mime=audio%2Fwebm&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&mv=m&sparams=clen%2Cdur%2Cei%2Cgir%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Ckeepalive%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Crequiressl%2Csource%2Cexpire&mt=1510033130&itag=249&keepalive=yes&key=yt6&signature=5E03B259A79D263CC941BB818AD76D1392494842.4A7F2C91E3A2D45A4A33444D024549E900AA58FE&ei=hUcBWpj6IJWKcO6hq2g&expire=1510054885&source=youtube&dur=1205.421&clen=7512109&initcwndbps=1397500&requiressl=yes&lmt=1509220041875458&ipbits=0&ratebypass=yes", "protocol": "https", "format_note": "DASH audio", "format": "249 - audio only (DASH audio)", "ext": "webm", "format_id": "249", "player_url": null, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "none", "filesize": 7512109, "tbr": 57.775, "acodec": "opus", "abr": 50}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mn=sn-4g5ednld&mm=31&ip=149.172.215.29&gir=yes&pl=17&mime=audio%2Fwebm&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&mv=m&sparams=clen%2Cdur%2Cei%2Cgir%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Ckeepalive%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Crequiressl%2Csource%2Cexpire&mt=1510033130&itag=250&keepalive=yes&key=yt6&signature=2C888AF2F17AFAFB706727068DB3F3FDF8AF1D09.480541EE5E5FD6DCC989A467EC48680DDE0030EE&ei=hUcBWpj6IJWKcO6hq2g&expire=1510054885&source=youtube&dur=1205.421&clen=9800616&initcwndbps=1397500&requiressl=yes&lmt=1509220081018978&ipbits=0&ratebypass=yes", "protocol": "https", "format_note": "DASH audio", "format": "250 - audio only (DASH audio)", "ext": "webm", "format_id": "250", "player_url": null, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "none", "filesize": 9800616, "tbr": 78.232, "acodec": "opus", "abr": 70}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mn=sn-4g5ednld&mm=31&ip=149.172.215.29&gir=yes&pl=17&mime=audio%2Fwebm&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&mv=m&sparams=clen%2Cdur%2Cei%2Cgir%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Ckeepalive%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Crequiressl%2Csource%2Cexpire&mt=1510033130&itag=171&keepalive=yes&key=yt6&signature=579E5C3854EFE77C5EF5A7B7D0562DA1740248FA.53A87A7BBFF2E0FAAB1432926DA057A74D2FB9FF&ei=hUcBWpj6IJWKcO6hq2g&expire=1510054885&source=youtube&dur=1205.389&clen=15461858&initcwndbps=1397500&requiressl=yes&lmt=1509220115324822&ipbits=0&ratebypass=yes", "protocol": "https", "format_note": "DASH audio", "format": "171 - audio only (DASH audio)", "ext": "webm", "format_id": "171", "player_url": null, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "none", "filesize": 15461858, "tbr": 120.117, "acodec": "vorbis", "abr": 128}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?id=e48c10d001af7ee4&itag=140&source=youtube&requiressl=yes&mn=sn-4g5ednld&ms=au&mv=m&ei=hkcBWs3jEIvscp-5gagE&pl=17&mm=31&initcwndbps=1311250&ratebypass=yes&mime=audio/mp4&gir=yes&clen=19146176&lmt=1510029849797209&dur=1205.440&key=dg_yt0&signature=2BFB0E3C233101160B6524DFFE26D17ADF2EB1DA.995E6EA9287F98B9ED86A7CBA773F37159415E42&mt=1510033130&ip=149.172.215.29&ipbits=0&expire=1510054886&sparams=ip,ipbits,expire,id,itag,source,requiressl,mn,ms,mv,ei,pl,mm,initcwndbps,ratebypass,mime,gir,clen,lmt,dur", "protocol": "https", "format_note": "DASH audio", "format": "140 - audio only (DASH audio)", "ext": "m4a", "format_id": "140", "width": null, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "height": null, "asr": 44100, "fps": null, "manifest_url": "https://manifest.googlevideo.com/api/manifest/dash/id/e48c10d001af7ee4/requiressl/yes/key/yt6/mn/sn-4g5ednld/expire/1510054886/sparams/as%2Cei%2Chfr%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Cplayback_host%2Crequiressl%2Csource%2Cexpire/source/youtube/itag/0/ms/au/playback_host/r4---sn-4g5ednld.googlevideo.com/mv/m/ei/hkcBWs3jEIvscp-5gagE/signature/E3C9CB7E639DAC1F379DD3286CFB5219BEC1DF56.16DAB28033C7AC96D8875B4DF9A3FBAEBB4E05D7/pl/17/ipbits/0/mm/31/hfr/1/initcwndbps/1311250/ip/149.172.215.29/mt/1510033130/as/fmp4_audio_clear%2Cfmp4_sd_hd_clear", "language": null, "container": "m4a_dash", "vcodec": "none", "filesize": 19146176, "tbr": 129.139, "acodec": "mp4a.40.2", "abr": 128}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mn=sn-4g5ednld&mm=31&ip=149.172.215.29&gir=yes&pl=17&mime=audio%2Fwebm&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&mv=m&sparams=clen%2Cdur%2Cei%2Cgir%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Ckeepalive%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Crequiressl%2Csource%2Cexpire&mt=1510033130&itag=251&keepalive=yes&key=yt6&signature=63C4DCAEE40A17284F3F531D872D25281C0E4A21.680B6BEB8931E76A38D27FED5EA749478FA35B34&ei=hUcBWpj6IJWKcO6hq2g&expire=1510054885&source=youtube&dur=1205.421&clen=19152907&initcwndbps=1397500&requiressl=yes&lmt=1509220063342381&ipbits=0&ratebypass=yes", "protocol": "https", "format_note": "DASH audio", "format": "251 - audio only (DASH audio)", "ext": "webm", "format_id": "251", "player_url": null, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "none", "filesize": 19152907, "tbr": 148.79, "acodec": "opus", "abr": 160}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mn=sn-4g5ednld&mm=31&ip=149.172.215.29&gir=yes&pl=17&aitags=133%2C134%2C135%2C136%2C137%2C160%2C242%2C243%2C244%2C247%2C248%2C278%2C298%2C299%2C302%2C303&mime=video%2Fwebm&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&mv=m&sparams=aitags%2Cclen%2Cdur%2Cei%2Cgir%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Ckeepalive%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Crequiressl%2Csource%2Cexpire&mt=1510033130&itag=278&keepalive=yes&key=yt6&signature=97854548DE12D3E7407A9FDDB39B82BFE4513327.0D03B81AD8F85F74055764AA18F86CFD72FEA551&ei=hUcBWpj6IJWKcO6hq2g&expire=1510054885&source=youtube&dur=1205.400&clen=12758731&initcwndbps=1397500&requiressl=yes&lmt=1509220811525506&ipbits=0&ratebypass=yes", "filesize": 12758731, "protocol": "https", "format_note": "144p", "format": "278 - 256x144 (144p)", "container": "webm", "ext": "webm", "fps": 30, "format_id": "278", "player_url": null, "width": 256, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "vp9", "height": 144, "tbr": 97.225, "acodec": "none"}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?id=e48c10d001af7ee4&itag=160&source=youtube&requiressl=yes&mn=sn-4g5ednld&ms=au&mv=m&ei=hkcBWs3jEIvscp-5gagE&pl=17&mm=31&initcwndbps=1311250&ratebypass=yes&mime=video/mp4&gir=yes&clen=7406322&lmt=1510029659931569&dur=1205.399&key=dg_yt0&signature=1CF2E5077EBD96490D5E2EFD8B9866A33C51ADC4.677565442F488BE306AFA7D124A660AD5E8BFEEC&mt=1510033130&ip=149.172.215.29&ipbits=0&expire=1510054886&sparams=ip,ipbits,expire,id,itag,source,requiressl,mn,ms,mv,ei,pl,mm,initcwndbps,ratebypass,mime,gir,clen,lmt,dur", "protocol": "https", "format_note": "DASH video", "format": "160 - 256x144 (DASH video)", "ext": "mp4", "format_id": "160", "width": 256, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "height": 144, "fps": 30, "manifest_url": "https://manifest.googlevideo.com/api/manifest/dash/id/e48c10d001af7ee4/requiressl/yes/key/yt6/mn/sn-4g5ednld/expire/1510054886/sparams/as%2Cei%2Chfr%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Cplayback_host%2Crequiressl%2Csource%2Cexpire/source/youtube/itag/0/ms/au/playback_host/r4---sn-4g5ednld.googlevideo.com/mv/m/ei/hkcBWs3jEIvscp-5gagE/signature/E3C9CB7E639DAC1F379DD3286CFB5219BEC1DF56.16DAB28033C7AC96D8875B4DF9A3FBAEBB4E05D7/pl/17/ipbits/0/mm/31/hfr/1/initcwndbps/1311250/ip/149.172.215.29/mt/1510033130/as/fmp4_audio_clear%2Cfmp4_sd_hd_clear", "language": null, "vcodec": "avc1.4d400c", "filesize": 7406322, "tbr": 111.559, "acodec": "none", "asr": null}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mn=sn-4g5ednld&mm=31&ip=149.172.215.29&gir=yes&pl=17&aitags=133%2C134%2C135%2C136%2C137%2C160%2C242%2C243%2C244%2C247%2C248%2C278%2C298%2C299%2C302%2C303&mime=video%2Fwebm&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&mv=m&sparams=aitags%2Cclen%2Cdur%2Cei%2Cgir%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Ckeepalive%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Crequiressl%2Csource%2Cexpire&mt=1510033130&itag=242&keepalive=yes&key=yt6&signature=82A62FC3B3E86E188D40E8A6170A3D911E6AFDE2.9B2F792A2CC2E930F7259CE6FD1EB6874C4A2F84&ei=hUcBWpj6IJWKcO6hq2g&expire=1510054885&source=youtube&dur=1205.400&clen=25918864&initcwndbps=1397500&requiressl=yes&lmt=1509220810865334&ipbits=0&ratebypass=yes", "height": 240, "format_note": "240p", "format": "242 - 426x240 (240p)", "protocol": "https", "ext": "webm", "fps": 30, "format_id": "242", "player_url": null, "width": 426, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "vp9", "filesize": 25918864, "tbr": 225.081, "acodec": "none"}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?id=e48c10d001af7ee4&itag=133&source=youtube&requiressl=yes&mn=sn-4g5ednld&ms=au&mv=m&ei=hkcBWs3jEIvscp-5gagE&pl=17&mm=31&initcwndbps=1311250&ratebypass=yes&mime=video/mp4&gir=yes&clen=14818821&lmt=1510029659970337&dur=1205.399&key=dg_yt0&signature=16C5A8DA2D78001CCC0CD6BE6F85662A4B7C6D0D.6FF174121D9B21E48A9D377F422FF797456F8FE5&mt=1510033130&ip=149.172.215.29&ipbits=0&expire=1510054886&sparams=ip,ipbits,expire,id,itag,source,requiressl,mn,ms,mv,ei,pl,mm,initcwndbps,ratebypass,mime,gir,clen,lmt,dur", "protocol": "https", "format_note": "DASH video", "format": "133 - 426x240 (DASH video)", "ext": "mp4", "format_id": "133", "width": 426, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "height": 240, "fps": 30, "manifest_url": "https://manifest.googlevideo.com/api/manifest/dash/id/e48c10d001af7ee4/requiressl/yes/key/yt6/mn/sn-4g5ednld/expire/1510054886/sparams/as%2Cei%2Chfr%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Cplayback_host%2Crequiressl%2Csource%2Cexpire/source/youtube/itag/0/ms/au/playback_host/r4---sn-4g5ednld.googlevideo.com/mv/m/ei/hkcBWs3jEIvscp-5gagE/signature/E3C9CB7E639DAC1F379DD3286CFB5219BEC1DF56.16DAB28033C7AC96D8875B4DF9A3FBAEBB4E05D7/pl/17/ipbits/0/mm/31/hfr/1/initcwndbps/1311250/ip/149.172.215.29/mt/1510033130/as/fmp4_audio_clear%2Cfmp4_sd_hd_clear", "language": null, "vcodec": "avc1.4d4015", "filesize": 14818821, "tbr": 245.045, "acodec": "none", "asr": null}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mn=sn-4g5ednld&mm=31&ip=149.172.215.29&gir=yes&pl=17&aitags=133%2C134%2C135%2C136%2C137%2C160%2C242%2C243%2C244%2C247%2C248%2C278%2C298%2C299%2C302%2C303&mime=video%2Fwebm&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&mv=m&sparams=aitags%2Cclen%2Cdur%2Cei%2Cgir%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Ckeepalive%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Crequiressl%2Csource%2Cexpire&mt=1510033130&itag=243&keepalive=yes&key=yt6&signature=3E874ACE17843E53F22764FBAF7CF68B712F4DC9.1B139B947897785AC0E736AD5A26ED0EAD4462ED&ei=hUcBWpj6IJWKcO6hq2g&expire=1510054885&source=youtube&dur=1205.400&clen=49665778&initcwndbps=1397500&requiressl=yes&lmt=1509220812356164&ipbits=0&ratebypass=yes", "height": 360, "format_note": "360p", "format": "243 - 640x360 (360p)", "protocol": "https", "ext": "webm", "fps": 30, "format_id": "243", "player_url": null, "width": 640, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "vp9", "filesize": 49665778, "tbr": 415.981, "acodec": "none"}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?id=e48c10d001af7ee4&itag=134&source=youtube&requiressl=yes&mn=sn-4g5ednld&ms=au&mv=m&ei=hkcBWs3jEIvscp-5gagE&pl=17&mm=31&initcwndbps=1311250&ratebypass=yes&mime=video/mp4&gir=yes&clen=40277006&lmt=1510029659922435&dur=1205.399&key=dg_yt0&signature=6F320795FBD46227C824B218031591BDA3A07A0C.766E6873293147D07518D0317C7025DC3B1490D9&mt=1510033130&ip=149.172.215.29&ipbits=0&expire=1510054886&sparams=ip,ipbits,expire,id,itag,source,requiressl,mn,ms,mv,ei,pl,mm,initcwndbps,ratebypass,mime,gir,clen,lmt,dur", "protocol": "https", "format_note": "DASH video", "format": "134 - 640x360 (DASH video)", "ext": "mp4", "format_id": "134", "width": 640, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "height": 360, "fps": 30, "manifest_url": "https://manifest.googlevideo.com/api/manifest/dash/id/e48c10d001af7ee4/requiressl/yes/key/yt6/mn/sn-4g5ednld/expire/1510054886/sparams/as%2Cei%2Chfr%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Cplayback_host%2Crequiressl%2Csource%2Cexpire/source/youtube/itag/0/ms/au/playback_host/r4---sn-4g5ednld.googlevideo.com/mv/m/ei/hkcBWs3jEIvscp-5gagE/signature/E3C9CB7E639DAC1F379DD3286CFB5219BEC1DF56.16DAB28033C7AC96D8875B4DF9A3FBAEBB4E05D7/pl/17/ipbits/0/mm/31/hfr/1/initcwndbps/1311250/ip/149.172.215.29/mt/1510033130/as/fmp4_audio_clear%2Cfmp4_sd_hd_clear", "language": null, "vcodec": "avc1.4d401e", "filesize": 40277006, "tbr": 635.454, "acodec": "none", "asr": null}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mn=sn-4g5ednld&mm=31&ip=149.172.215.29&gir=yes&pl=17&aitags=133%2C134%2C135%2C136%2C137%2C160%2C242%2C243%2C244%2C247%2C248%2C278%2C298%2C299%2C302%2C303&mime=video%2Fwebm&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&mv=m&sparams=aitags%2Cclen%2Cdur%2Cei%2Cgir%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Ckeepalive%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Crequiressl%2Csource%2Cexpire&mt=1510033130&itag=244&keepalive=yes&key=yt6&signature=CD835C97A6B63AA40DC139A77CD79F8D094A1ED1.27F22634020C8419596FCD6F5385025EDAD1EED4&ei=hUcBWpj6IJWKcO6hq2g&expire=1510054885&source=youtube&dur=1205.400&clen=89031435&initcwndbps=1397500&requiressl=yes&lmt=1509220818019876&ipbits=0&ratebypass=yes", "height": 480, "format_note": "480p", "format": "244 - 854x480 (480p)", "protocol": "https", "ext": "webm", "fps": 30, "format_id": "244", "player_url": null, "width": 854, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "vp9", "filesize": 89031435, "tbr": 763.921, "acodec": "none"}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?id=e48c10d001af7ee4&itag=135&source=youtube&requiressl=yes&mn=sn-4g5ednld&ms=au&mv=m&ei=hkcBWs3jEIvscp-5gagE&pl=17&mm=31&initcwndbps=1311250&ratebypass=yes&mime=video/mp4&gir=yes&clen=83197723&lmt=1510029660126496&dur=1205.399&key=dg_yt0&signature=36B57731F1A8AB2F55FFD8928440DDBED36FC3DB.969F35671363277D50A0E630032D1B3398F5A5A4&mt=1510033130&ip=149.172.215.29&ipbits=0&expire=1510054886&sparams=ip,ipbits,expire,id,itag,source,requiressl,mn,ms,mv,ei,pl,mm,initcwndbps,ratebypass,mime,gir,clen,lmt,dur", "protocol": "https", "format_note": "DASH video", "format": "135 - 854x480 (DASH video)", "ext": "mp4", "format_id": "135", "width": 854, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "height": 480, "fps": 30, "manifest_url": "https://manifest.googlevideo.com/api/manifest/dash/id/e48c10d001af7ee4/requiressl/yes/key/yt6/mn/sn-4g5ednld/expire/1510054886/sparams/as%2Cei%2Chfr%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Cplayback_host%2Crequiressl%2Csource%2Cexpire/source/youtube/itag/0/ms/au/playback_host/r4---sn-4g5ednld.googlevideo.com/mv/m/ei/hkcBWs3jEIvscp-5gagE/signature/E3C9CB7E639DAC1F379DD3286CFB5219BEC1DF56.16DAB28033C7AC96D8875B4DF9A3FBAEBB4E05D7/pl/17/ipbits/0/mm/31/hfr/1/initcwndbps/1311250/ip/149.172.215.29/mt/1510033130/as/fmp4_audio_clear%2Cfmp4_sd_hd_clear", "language": null, "vcodec": "avc1.4d401f", "filesize": 83197723, "tbr": 1190.586, "acodec": "none", "asr": null}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mn=sn-4g5ednld&mm=31&ip=149.172.215.29&gir=yes&pl=17&aitags=133%2C134%2C135%2C136%2C137%2C160%2C242%2C243%2C244%2C247%2C248%2C278%2C298%2C299%2C302%2C303&mime=video%2Fwebm&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&mv=m&sparams=aitags%2Cclen%2Cdur%2Cei%2Cgir%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Ckeepalive%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Crequiressl%2Csource%2Cexpire&mt=1510033130&itag=247&keepalive=yes&key=yt6&signature=5776A02A134302997FE0EF2712B76C7CEDFF64A2.DCD2B7B001B563E883C96118854AB93B664B7014&ei=hUcBWpj6IJWKcO6hq2g&expire=1510054885&source=youtube&dur=1205.400&clen=181684548&initcwndbps=1397500&requiressl=yes&lmt=1509220822466355&ipbits=0&ratebypass=yes", "height": 720, "format_note": "720p", "format": "247 - 1280x720 (720p)", "protocol": "https", "ext": "webm", "fps": 30, "format_id": "247", "player_url": null, "width": 1280, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "vp9", "filesize": 181684548, "tbr": 1518.869, "acodec": "none"}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mn=sn-4g5ednld&mm=31&ip=149.172.215.29&gir=yes&pl=17&aitags=133%2C134%2C135%2C136%2C137%2C160%2C242%2C243%2C244%2C247%2C248%2C278%2C298%2C299%2C302%2C303&mime=video%2Fmp4&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&mv=m&sparams=aitags%2Cclen%2Cdur%2Cei%2Cgir%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Ckeepalive%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Crequiressl%2Csource%2Cexpire&mt=1510033130&itag=136&keepalive=yes&key=yt6&signature=48CB5B7630E9B2EF064FEAA90283C93600C8C99A.97A6FF092B50C45B93A9D00A4489951CAC420264&ei=hUcBWpj6IJWKcO6hq2g&expire=1510054885&source=youtube&dur=1205.399&clen=159409255&initcwndbps=1397500&requiressl=yes&lmt=1510029660038529&ipbits=0&ratebypass=yes", "height": 720, "format_note": "720p", "format": "136 - 1280x720 (720p)", "protocol": "https", "ext": "mp4", "fps": 30, "format_id": "136", "player_url": null, "width": 1280, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "avc1.4d401f", "filesize": 159409255, "tbr": 2426.878, "acodec": "none"}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mn=sn-4g5ednld&mm=31&ip=149.172.215.29&gir=yes&pl=17&aitags=133%2C134%2C135%2C136%2C137%2C160%2C242%2C243%2C244%2C247%2C248%2C278%2C298%2C299%2C302%2C303&mime=video%2Fwebm&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&mv=m&sparams=aitags%2Cclen%2Cdur%2Cei%2Cgir%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Ckeepalive%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Crequiressl%2Csource%2Cexpire&mt=1510033130&itag=302&keepalive=yes&key=yt6&signature=63C83F206363E38FCBEE6FD169C841E94D3F9E75.B5FA9E01794F479B511E3C00DED483C43A18079F&ei=hUcBWpj6IJWKcO6hq2g&expire=1510054885&source=youtube&dur=1205.383&clen=287851935&initcwndbps=1397500&requiressl=yes&lmt=1509220334930529&ipbits=0&ratebypass=yes", "filesize": 287851935, "protocol": "https", "format_note": "720p60", "format": "302 - 1280x720 (720p60)", "ext": "webm", "fps": 60, "format_id": "302", "player_url": null, "width": 1280, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "vp9", "height": 720, "tbr": 2655.856, "acodec": "none"}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mn=sn-4g5ednld&mm=31&ip=149.172.215.29&gir=yes&pl=17&aitags=133%2C134%2C135%2C136%2C137%2C160%2C242%2C243%2C244%2C247%2C248%2C278%2C298%2C299%2C302%2C303&mime=video%2Fwebm&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&mv=m&sparams=aitags%2Cclen%2Cdur%2Cei%2Cgir%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Ckeepalive%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Crequiressl%2Csource%2Cexpire&mt=1510033130&itag=248&keepalive=yes&key=yt6&signature=4DA17D83E791B0C197A2D3967D7A75F59CE8F25A.2309F16DF7609A5254AAA09E1E8233DB1D25C3C8&ei=hUcBWpj6IJWKcO6hq2g&expire=1510054885&source=youtube&dur=1205.400&clen=333440838&initcwndbps=1397500&requiressl=yes&lmt=1509220321093101&ipbits=0&ratebypass=yes", "height": 1080, "format_note": "1080p", "format": "248 - 1920x1080 (1080p)", "protocol": "https", "ext": "webm", "fps": 30, "format_id": "248", "player_url": null, "width": 1920, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "vp9", "filesize": 333440838, "tbr": 2688.337, "acodec": "none"}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?id=e48c10d001af7ee4&itag=298&source=youtube&requiressl=yes&mn=sn-4g5ednld&ms=au&mv=m&ei=hkcBWs3jEIvscp-5gagE&pl=17&mm=31&initcwndbps=1311250&ratebypass=yes&mime=video/mp4&gir=yes&clen=259181823&lmt=1510029638957866&dur=1205.383&key=dg_yt0&signature=7B07F1D7FFAB14B4EDC22B12C7F5D83CD1491062.5CDB814AB2D30C985AF2DB6773CE9E7875EDD0AE&mt=1510033130&ip=149.172.215.29&ipbits=0&expire=1510054886&sparams=ip,ipbits,expire,id,itag,source,requiressl,mn,ms,mv,ei,pl,mm,initcwndbps,ratebypass,mime,gir,clen,lmt,dur", "protocol": "https", "format_note": "DASH video", "format": "298 - 1280x720 (DASH video)", "ext": "mp4", "format_id": "298", "width": 1280, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "height": 720, "fps": 60, "manifest_url": "https://manifest.googlevideo.com/api/manifest/dash/id/e48c10d001af7ee4/requiressl/yes/key/yt6/mn/sn-4g5ednld/expire/1510054886/sparams/as%2Cei%2Chfr%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Cplayback_host%2Crequiressl%2Csource%2Cexpire/source/youtube/itag/0/ms/au/playback_host/r4---sn-4g5ednld.googlevideo.com/mv/m/ei/hkcBWs3jEIvscp-5gagE/signature/E3C9CB7E639DAC1F379DD3286CFB5219BEC1DF56.16DAB28033C7AC96D8875B4DF9A3FBAEBB4E05D7/pl/17/ipbits/0/mm/31/hfr/1/initcwndbps/1311250/ip/149.172.215.29/mt/1510033130/as/fmp4_audio_clear%2Cfmp4_sd_hd_clear", "language": null, "vcodec": "avc1.4d4020", "filesize": 259181823, "tbr": 3464.832, "acodec": "none", "asr": null}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mn=sn-4g5ednld&mm=31&ip=149.172.215.29&gir=yes&pl=17&aitags=133%2C134%2C135%2C136%2C137%2C160%2C242%2C243%2C244%2C247%2C248%2C278%2C298%2C299%2C302%2C303&mime=video%2Fmp4&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&mv=m&sparams=aitags%2Cclen%2Cdur%2Cei%2Cgir%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Ckeepalive%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Crequiressl%2Csource%2Cexpire&mt=1510033130&itag=137&keepalive=yes&key=yt6&signature=28189F4C643C09EA708112A0389C37A5F8847C8A.B9487C1C17657C09E51D914ADDA8D58D1D54E31F&ei=hUcBWpj6IJWKcO6hq2g&expire=1510054885&source=youtube&dur=1205.399&clen=293453011&initcwndbps=1397500&requiressl=yes&lmt=1510029643904414&ipbits=0&ratebypass=yes", "height": 1080, "format_note": "1080p", "format": "137 - 1920x1080 (1080p)", "protocol": "https", "ext": "mp4", "fps": 30, "format_id": "137", "player_url": null, "width": 1920, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "avc1.640028", "filesize": 293453011, "tbr": 4322.702, "acodec": "none"}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mn=sn-4g5ednld&mm=31&ip=149.172.215.29&gir=yes&pl=17&aitags=133%2C134%2C135%2C136%2C137%2C160%2C242%2C243%2C244%2C247%2C248%2C278%2C298%2C299%2C302%2C303&mime=video%2Fwebm&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&mv=m&sparams=aitags%2Cclen%2Cdur%2Cei%2Cgir%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Ckeepalive%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Crequiressl%2Csource%2Cexpire&mt=1510033130&itag=303&keepalive=yes&key=yt6&signature=62EF76A63A335734356F809480D0A388380C8E69.765AE7BD2F90530A4E906B7CABF4A28C207C84A1&ei=hUcBWpj6IJWKcO6hq2g&expire=1510054885&source=youtube&dur=1205.383&clen=542453753&initcwndbps=1397500&requiressl=yes&lmt=1509220755544739&ipbits=0&ratebypass=yes", "filesize": 542453753, "protocol": "https", "format_note": "1080p60", "format": "303 - 1920x1080 (1080p60)", "ext": "webm", "fps": 60, "format_id": "303", "player_url": null, "width": 1920, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "vp9", "height": 1080, "tbr": 4442.198, "acodec": "none"}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?id=e48c10d001af7ee4&itag=299&source=youtube&requiressl=yes&mn=sn-4g5ednld&ms=au&mv=m&ei=hkcBWs3jEIvscp-5gagE&pl=17&mm=31&initcwndbps=1311250&ratebypass=yes&mime=video/mp4&gir=yes&clen=515535482&lmt=1510029653727916&dur=1205.383&key=dg_yt0&signature=1702D702941E3F03DBBA58CDF9E9F59A51031584.2B1D90096C036ECF749B843F65F4DEF4939F0667&mt=1510033130&ip=149.172.215.29&ipbits=0&expire=1510054886&sparams=ip,ipbits,expire,id,itag,source,requiressl,mn,ms,mv,ei,pl,mm,initcwndbps,ratebypass,mime,gir,clen,lmt,dur", "protocol": "https", "format_note": "DASH video", "format": "299 - 1920x1080 (DASH video)", "ext": "mp4", "format_id": "299", "width": 1920, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "height": 1080, "fps": 60, "manifest_url": "https://manifest.googlevideo.com/api/manifest/dash/id/e48c10d001af7ee4/requiressl/yes/key/yt6/mn/sn-4g5ednld/expire/1510054886/sparams/as%2Cei%2Chfr%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Cplayback_host%2Crequiressl%2Csource%2Cexpire/source/youtube/itag/0/ms/au/playback_host/r4---sn-4g5ednld.googlevideo.com/mv/m/ei/hkcBWs3jEIvscp-5gagE/signature/E3C9CB7E639DAC1F379DD3286CFB5219BEC1DF56.16DAB28033C7AC96D8875B4DF9A3FBAEBB4E05D7/pl/17/ipbits/0/mm/31/hfr/1/initcwndbps/1311250/ip/149.172.215.29/mt/1510033130/as/fmp4_audio_clear%2Cfmp4_sd_hd_clear", "language": null, "vcodec": "avc1.64002a", "filesize": 515535482, "tbr": 6644.16, "acodec": "none", "asr": null}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mn=sn-4g5ednld&mm=31&ip=149.172.215.29&gir=yes&pl=17&mime=video%2F3gpp&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&mv=m&sparams=clen%2Cdur%2Cei%2Cgir%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Crequiressl%2Csource%2Cexpire&mt=1510033130&itag=17&key=yt6&signature=C968E1FB66D8DDAC93765DD3B1812379EE27D178.32BC520DD34447841F407AA82AF801E9866BEE4F&ei=hUcBWpj6IJWKcO6hq2g&expire=1510054885&source=youtube&dur=1205.487&clen=12043617&initcwndbps=1397500&requiressl=yes&lmt=1510026573930720&ipbits=0&ratebypass=yes", "protocol": "https", "format_note": "small", "format": "17 - 176x144 (small)", "resolution": "176x144", "ext": "3gp", "format_id": "17", "player_url": null, "width": 176, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "mp4v.20.3", "height": 144, "acodec": "mp4a.40.2", "abr": 24}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mn=sn-4g5ednld&mm=31&ip=149.172.215.29&gir=yes&pl=17&mime=video%2F3gpp&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&mv=m&sparams=clen%2Cdur%2Cei%2Cgir%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Crequiressl%2Csource%2Cexpire&mt=1510033130&itag=36&key=yt6&signature=5712D902EDB358A4A6E6DEE63A8E75B98DE043F1.D201F67D0879E6385B4829F7FB92A11D3C0ADC46&ei=hUcBWpj6IJWKcO6hq2g&expire=1510054885&source=youtube&dur=1205.487&clen=35676271&initcwndbps=1397500&requiressl=yes&lmt=1510026579839939&ipbits=0&ratebypass=yes", "protocol": "https", "format_note": "small", "format": "36 - 320x180 (small)", "resolution": "320x180", "ext": "3gp", "format_id": "36", "player_url": null, "width": 320, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "mp4v.20.3", "height": 180, "acodec": "mp4a.40.2"}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mn=sn-4g5ednld&mm=31&ip=149.172.215.29&gir=yes&pl=17&mime=video%2Fwebm&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&mv=m&sparams=clen%2Cdur%2Cei%2Cgir%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Cratebypass%2Crequiressl%2Csource%2Cexpire&mt=1510033130&itag=43&key=yt6&signature=31B81408DCF637DCF2C7FFA7F2E78BD19A8FFB2A.BB5B211BB5ACA526458943F34EB4EA1BB3A9EDB4&ei=hUcBWpj6IJWKcO6hq2g&expire=1510054885&source=youtube&dur=0.000&clen=99457955&initcwndbps=1397500&requiressl=yes&ratebypass=yes&lmt=1510032726201608&ipbits=0", "protocol": "https", "format_note": "medium", "format": "43 - 640x360 (medium)", "resolution": "640x360", "ext": "webm", "format_id": "43", "player_url": null, "width": 640, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "vp8.0", "height": 360, "acodec": "vorbis", "abr": 128}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mn=sn-4g5ednld&mm=31&ip=149.172.215.29&gir=yes&pl=17&mime=video%2Fmp4&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&mv=m&sparams=clen%2Cdur%2Cei%2Cgir%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Cratebypass%2Crequiressl%2Csource%2Cexpire&mt=1510033130&itag=18&key=yt6&signature=6D95A7B871CEE1D3754C749E010FA773BED56DB4.ABB9AF86C5DD9BC294881108C6F4B43973A0AEF6&ei=hUcBWpj6IJWKcO6hq2g&expire=1510054885&source=youtube&dur=1205.440&clen=90380839&initcwndbps=1397500&requiressl=yes&ratebypass=yes&lmt=1510029875775472&ipbits=0", "protocol": "https", "format_note": "medium", "format": "18 - 640x360 (medium)", "resolution": "640x360", "ext": "mp4", "format_id": "18", "player_url": null, "width": 640, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "avc1.42001E", "height": 360, "acodec": "mp4a.40.2", "abr": 96}, {"url": "https://r4---sn-4g5ednld.googlevideo.com/videoplayback?mt=1510033130&mn=sn-4g5ednld&mm=31&ip=149.172.215.29&pl=17&mime=video%2Fmp4&id=o-AP2kp0GjaKlvNv0sk93224RDYXVJqnvfZgxe9h51DM_Q&ms=au&initcwndbps=1397500&mv=m&sparams=dur%2Cei%2Cid%2Cinitcwndbps%2Cip%2Cipbits%2Citag%2Clmt%2Cmime%2Cmm%2Cmn%2Cms%2Cmv%2Cpl%2Cratebypass%2Crequiressl%2Csource%2Cexpire&requiressl=yes&itag=22&ratebypass=yes&lmt=1510030215409299&key=yt6&signature=08ECBD1DE74B8ADD925F79696769C76D29C4AB71.6999C2F92DAD11E994B79D1914B4F543BA8C2A0B&ei=hUcBWpj6IJWKcO6hq2g&dur=1205.440&expire=1510054885&source=youtube&ipbits=0", "protocol": "https", "format_note": "hd720", "format": "22 - 1280x720 (hd720)", "resolution": "1280x720", "ext": "mp4", "format_id": "22", "player_url": null, "width": 1280, "http_headers": {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20150101 Firefox/47.0 (Chrome)", "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7", "Accept-Language": "en-us,en;q=0.5", "Accept-Encoding": "gzip, deflate"}, "vcodec": "avc1.64001F", "height": 720, "acodec": "mp4a.40.2", "abr": 192}], "chapters": null, "_filename": "StarCraft - Remastered - UNITED EARTH DIRECTORATE!-5IwQ0AGvfuQ.mp4", "tags": [], "duration": 1205, "id": "5IwQ0AGvfuQ", "average_rating": 4.95571947098, "format_id": "299+251", "resolution": null, "vcodec": "avc1.64002a", "age_limit": 0, "creator": null, "fps": 60, "automatic_captions": {}, "playlist": null, "vbr": null, "uploader": "LowkoTV", "license": null, "display_id": "5IwQ0AGvfuQ", "view_count": 19264, "stretched_ratio": null, "playlist_index": null, "abr": 160, "annotations": null}

    VMVod vod;
    VMVodData& data = vod.data();

    QJsonObject root = doc.object();
    data._webPageUrl = QUrl(root.value(QStringLiteral("webpage_url")).toString());
    if (data._webPageUrl.isEmpty()) {
        QString message = QStringLiteral("Invalid youtube-dl JSON response: ") + output.data();
        qCritical() << message;
        result["message"] = message;
        emit metaDataFetchComplete(id, result);
        return;
    }

    data._fullTitle = root.value(QStringLiteral("fulltitle")).toString();
    data._title = root.value(QStringLiteral("title")).toString();

    //data.fileExtension = root.value(QStringLiteral("ext")).toString();
    data.durationS = root.value("duration").toInt();
    QJsonArray thumbnails = root.value(QStringLiteral("thumbnails")).toArray();
    if (thumbnails.size()) {
        QJsonObject first = thumbnails[0].toObject();
        data._thumbnail = first.value("0").toString();
        if (data._thumbnail.isEmpty() && !first.keys().isEmpty()) {
            data._thumbnail = first.constBegin().value().toString();
        }
    }

    QJsonArray formats = root.value(QStringLiteral("formats")).toArray();
    for (int i = 0; i < formats.size(); ++i) {
        QJsonObject format = formats[i].toObject();
        QString vcodec = format.value(QStringLiteral("vcodec")).toString();
        QString acodec = format.value(QStringLiteral("acodec")).toString();
        if (vcodec.isEmpty() || QStringLiteral("none") == vcodec ||  // no video
            acodec.isEmpty() || QStringLiteral("none") == acodec) { // no audio
            continue;
        }

          //"format_id": "160p30",
        data._formats.append(VMVodFormat());
        VMVodFormat& vodFormat = data._formats.last();
        vodFormat._width = format.value(QStringLiteral("width")).toInt();
        vodFormat._height = format.value(QStringLiteral("height")).toInt();
        vodFormat._id = format.value(QStringLiteral("format_id")).toString();
        vodFormat._url = QUrl(format.value(QStringLiteral("url")).toString());
        vodFormat._displayName = format.value(QStringLiteral("format")).toString();
    }

    if (vod.formats().isEmpty()) {
        QString message = QStringLiteral("No format has video");
        result["message"] = message;
        emit metaDataFetchComplete(id, result);
    }

    result["vod"] = QVariant::fromValue(vod);

    {
        QMutexLocker g(&m_Lock);
        _response_cache.insert(qvariant_cast<QUrl>(result["request_url"]), new QVariantMap(result));
    }

    emit metaDataFetchComplete(id, result);


}
// https://www.youtube.com/watch?v=9Idy_WwS_fU
void
// e https://www.twitch.tv/videos/161472611?t=07h49m09s
VMVideoUrlFetcher::onProcessError(QProcess::ProcessError error)
{
    QProcess* process = qobject_cast<QProcess*>(QObject::sender());
    Q_ASSERT(process);

    qDebug() << "process ptr" << (void*)process;

    cleanupProcess(process);

    QMutexLocker g(&m_Lock);

    QVariantMap result;
    {
        result = _processDataMap.value(process);
        _processDataMap.remove(process);
        int id = result.value("id").toInt();
        _vodDownloads.remove(id);
    }

    result["error"] = error;
    qCritical() << "Process error:" << error;

    QString filePath = result["file_path"].toString();
    if (!filePath.isEmpty()) {
        // cleanup download
        QFile file (filePath);
        file.remove();
    }

    //FIX ME
    //emit failure(result);
}

void
VMVideoUrlFetcher::onVodProcessStarted() {
    QProcess* process = qobject_cast<QProcess*>(QObject::sender());
    Q_ASSERT(process);


}

void
VMVideoUrlFetcher::onVodProcessReadReady() {
    QProcess* process = qobject_cast<QProcess*>(QObject::sender());
    Q_ASSERT(process);

    qDebug() << "process ptr" << (void*)process;

    QMutexLocker g(&m_Lock);
    if (!_processDataMap.contains(process)) {
        qDebug() << "process" << process->processId() << "gone";
        return;
    }

    QVariantMap& result = *_processDataMap.find(process);
    if (!result.contains("id")) {
        qDebug() << "process" << process->processId() << "has no id?";
        return;
    }

    int id = result["id"].toInt();
    float progress = -1;

    QByteArray data = process->readAll();
    qDebug() << "read" << data.size() << "bytes";
    qDebug() << data;
    int start = 0, end = 0;
    while (end < data.size()) {
        if (data[end] == '\r' || data[end] == '\n') {
            if (start < end) {
                QString str = QString::fromLocal8Bit(data.data() + start, end - start);
                qDebug() << "line" << str;
                if (s_ProgressRegexp.indexIn(str) != -1) {
                    QString capture = s_ProgressRegexp.cap(1);
                    bool ok = false;
                    float value = capture.toFloat(&ok);
                    if (ok) {
                        qDebug() << "progress" << value;
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
        if (s_ProgressRegexp.indexIn(str) != -1) {
            QString capture = s_ProgressRegexp.cap(1);
            bool ok = false;
            float value = capture.toFloat(&ok);
            if (ok) {
                progress = value;
            }
        }
    }

    if (progress >= 0) {
        result["progress"] = qMin<float>(progress/100, 1);
        emit vodStatusChanged(id, result);
    }
}



QVariantMap
VMVideoUrlFetcher::parseResponse(QJsonDocument doc)
{
    Q_ASSERT(doc.isObject());
    QVariantMap map = doc.object().toVariantMap();

    if (!map.contains("formats")) {
        qCritical() << "Output JSON does not contain formats array";
        return QVariantMap();
    }

    QVariant formats = map["formats"];
    if (formats.type() != QVariant::List) {
        qCritical() << "Formats is not an array!" << formats.type();
        return QVariantMap();
    }

    QVariantMap response;
    QVariantList lst = formats.toList();
    QVariantList::iterator it = lst.begin();
    for (;it != lst.end(); ++it) {
        QVariantMap entry = it->toMap();
        if (entry.isEmpty())
            continue;
        if (!entry.contains("format_id") || !entry.contains("url"))
            continue;

        QVariantMap details;
        details["url"] = entry["url"];

        int itag = entry["format_id"].toInt();
        switch (itag) {
        case 18:
            response.insert("360p", details);
            break;
        case 22:
            response.insert("720p", details);
            break;
        case 37:
            response.insert(("1080p"), details);
            break;
        }
    }

    return response;
}

void
VMVideoUrlFetcher::dumpConnections() {
    QHash<QProcess*, QList<QMetaObject::Connection> >::const_iterator it = _connections.cbegin();
    while (it != _connections.cend()) {
        qDebug() << (void*)it.key();
        const QList<QMetaObject::Connection>& l = it.value();
        for (int i = 0; i < l.size(); ++i) {
            qDebug() << (bool)l[i];
        }
        ++it;
    }
}

void
VMVideoUrlFetcher::cleanupProcess(QProcess* process) {

    Q_ASSERT(process);
    dumpConnections();
    qDebug() << "disconnect" << (void*)process;
    process->disconnect();
    dumpConnections();

    process->deleteLater();
//    m_PendingProcessesWaitHandle.release();
//    --m_PendingProcesses;
}
