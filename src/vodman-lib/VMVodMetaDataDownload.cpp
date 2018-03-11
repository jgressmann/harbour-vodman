#include "VMVodMetaDataDownload.h"

//#include <QDBusMetaType>
#include <QDebug>
#include <QDebugStateSaver>
#include <QDataStream>

//namespace {
//int VMVodMetaDataDownloadRegistered = qDBusRegisterMetaType<VMVodMetaDataDownload>();
//}

VMVodMetaDataDownload::VMVodMetaDataDownload()
    : d(new VMVodMetaDataDownloadData())
{
}

//CREATE_ENUM_DATATYPE(VMVodMetaDataDownload::Result)

QDataStream &operator<<(QDataStream &stream, const VMVodMetaDataDownloadData &value) {
    stream << value._vod;
    stream << value.errorMessage;
    stream << value.error;
    stream << value._url;
    return stream;
}

QDataStream &operator>>(QDataStream &stream, VMVodMetaDataDownloadData &value) {
    stream >> value._vod;
    stream >> value.errorMessage;
    stream >> value.error;
    stream >> value._url;
    return stream;
}

QDataStream &operator<<(QDataStream &stream, const VMVodMetaDataDownload &value) {
    return stream << value.data();
}

QDataStream &operator>>(QDataStream &stream, VMVodMetaDataDownload &value) {
    return stream >> value.data();
}

//QDBusArgument& operator<<(QDBusArgument& arg, const VMVodMetaDataDownloadData& value) {
//    arg.beginStructure();
//    arg << value._vod;
//    arg << value._message;
//    arg << value._result;
//    arg << value._url;
//    arg.endStructure();
//    return arg;
//}


//const QDBusArgument& operator>>(const QDBusArgument& arg, VMVodMetaDataDownloadData& value) {
//    arg.beginStructure();
//    arg >> value._vod;
//    arg >> value._message;
//    arg >> value._result;
//    arg >> value._url;
//    arg.endStructure();
//    return arg;
//}

//QDBusArgument& operator<<(QDBusArgument& arg, const VMVodMetaDataDownload& value) {
//    arg << value.data();
//    return arg;
//}

//const QDBusArgument& operator>>(const QDBusArgument& arg, VMVodMetaDataDownload& value) {
//    arg >> value.data();
//    return arg;
//}

QDebug operator<<(QDebug debug, const VMVodMetaDataDownload& value) {
    const VMVodMetaDataDownloadData& data = value.data();
    QDebugStateSaver saver(debug);
    debug.nospace() << "VMVodMetaDataDownload("
                    << "error=" << data.error
                    << ", errorMessage=" << data.errorMessage
                    << ", url=" << data._url
                    << ", vod=" << data._vod
                    << ")";
    return debug;
}
