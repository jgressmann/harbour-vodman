#include "VMVodFileDownload.h"

//#include <QDBusMetaType>
#include <QDebug>
#include <QDebugStateSaver>
#include <QDataStream>

//namespace {
//int VMVodFileDownloadRegistered = qDBusRegisterMetaType<VMVodFileDownload>();
//}


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

//CREATE_ENUM_DATATYPE(VMVodFileDownload::Result)


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

//QDBusArgument& operator<<(QDBusArgument& arg, const VMVodFileDownloadData& value) {
//    arg.beginStructure();
//    arg << value._format;
//    arg << value._message;
////    arg << value._handle;
//    arg << value._progress;
//    arg << value._result;
//    arg << value._filePath;
//    arg.endStructure();
//    return arg;
//}


//const QDBusArgument& operator>>(const QDBusArgument& arg, VMVodFileDownloadData& value) {
//    arg.beginStructure();
//    arg >> value._format;
//    arg >> value._message;
////    arg >> value._handle;
//    arg >> value._progress;
//    arg >> value._result;
//    arg >> value._filePath;
//    arg.endStructure();
//    return arg;
//}

//QDBusArgument& operator<<(QDBusArgument& arg, const VMVodFileDownload& value) {
//    arg << value.data();
//    return arg;
//}

//const QDBusArgument& operator>>(const QDBusArgument& arg, VMVodFileDownload& value) {
//    arg >> value.data();
//    return arg;
//}

QDebug operator<<(QDebug debug, const VMVodFileDownload& value) {
    const VMVodFileDownloadData& data = value.data();
    QDebugStateSaver saver(debug);
    debug.nospace() << "VMVodFileDownload("
//                    << "url=" << value.url()
                    << "progress=" << data._progress
                    << ", state=" << data.error
                    << ", message=" << data.errorMessage
                    << ", filePath=" << data._filePath
                    << ", format=" << data.format
                    << ", desc=" << data.description
                    << ")";
    return debug;
}
