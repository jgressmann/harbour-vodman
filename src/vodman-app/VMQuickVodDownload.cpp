#include "VMQuickVodDownload.h"

VMQuickVodDownload::~VMQuickVodDownload()
{

}

VMQuickVodDownload::VMQuickVodDownload(QObject* parent)
    : QObject(parent)
{}


void VMQuickVodDownload::setData(const VMVodFileDownload& value) {
    m_Download = value;
    emit dataChanged();
//    emit progressChanged();
}
