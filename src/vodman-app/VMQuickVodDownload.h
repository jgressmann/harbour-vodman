#pragma once

#include "VMVodFileDownload.h"


class VMQuickVodDownload : public QObject
{
    Q_OBJECT
public:
    Q_PROPERTY(VMVodFileDownload data READ data NOTIFY dataChanged)
//    Q_PROPERTY(float progress READ progress NOTIFY progressChanged)
//    Q_PROPERTY(VMVod vod READ vod CONSTANT)
public:
    ~VMQuickVodDownload();
    explicit VMQuickVodDownload(QObject* parent = Q_NULLPTR);

//    int handle() const;
    VMVodFileDownload data() const { return m_Download; }
    void setData(const VMVodFileDownload& value);
//    float progress() const { return m_Download.progress(); }


signals:
    void dataChanged();
//    void progressChanged();

private:
    VMVodFileDownload m_Download;
};
