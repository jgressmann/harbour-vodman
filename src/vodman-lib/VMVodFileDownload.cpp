#include "VMVodFileDownload.h"


#include <QDebug>
#include <QDebugStateSaver>
#include <QDataStream>



bool VMVodFileDownloadRequest::isValid() const {
    return !filePath.isEmpty() && format.isValid() && description.isValid();
}


QDataStream &operator<<(QDataStream &stream, const VMVodFileDownloadRequest &value) {
    stream << value.description;
    stream << value.filePath;
    stream << value.format;
    return stream;
}

QDataStream &operator>>(QDataStream &stream, VMVodFileDownloadRequest &value) {
    stream >> value.description;
    stream >> value.filePath;
    stream >> value.format;
    return stream;
}

VMVodFileDownload::VMVodFileDownload()
    : d(new VMVodFileDownloadData())
{
}


bool VMVodFileDownload::isValid() const {
    return d->error == VMVodEnums::VM_ErrorNone &&
            d->format.isValid() &&
            d->description.isValid();
}

QDataStream &operator<<(QDataStream &stream, const VMVodFileDownloadData &value) {
    stream << value.format;
    stream << value.errorMessage;
    stream << value._progress;
    stream << value.error;
    stream << value._filePath;
    stream << value.description;
//    stream << value._url;
    return stream;
}

QDataStream &operator>>(QDataStream &stream, VMVodFileDownloadData &value) {
    stream >> value.format;
    stream >> value.errorMessage;
    stream >> value._progress;
    stream >> value.error;
    stream >> value._filePath;
    stream >> value.description;
//    stream >> value._url;
    return stream;
}

QDataStream &operator<<(QDataStream &stream, const VMVodFileDownload &value) {
    return stream << value.data();
}

QDataStream &operator>>(QDataStream &stream, VMVodFileDownload &value) {
    return stream >> value.data();
}


QDebug operator<<(QDebug debug, const VMVodFileDownload& value) {
    const VMVodFileDownloadData& data = value.data();
    QDebugStateSaver saver(debug);
    debug.nospace() << "VMVodFileDownload("
//                    << "url=" << value.url()
                    << "progress=" << data._progress
                    << ", error=" << data.error
                    << ", message=" << data.errorMessage
                    << ", filePath=" << data._filePath
                    << ", format=" << data.format
                    << ", desc=" << data.description
                    << ")";
    return debug;
}
