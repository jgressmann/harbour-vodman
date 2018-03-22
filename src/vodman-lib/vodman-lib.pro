TEMPLATE = lib
TARGET = vodman

QT -= gui
QT += core


CONFIG += console
CONFIG -= app_bundle

include(../../common.pri)



target.path = /usr/lib
INSTALLS += target



SOURCES += \
    VMVod.cpp \
    VMVodFileDownload.cpp \
    VMVodMetaDataDownload.cpp \

HEADERS += \
    VMVod.h \
    VMVodFileDownload.h \
    VMVodMetaDataDownload.h


#headers.path = $$DEPLOYMENT_PATH/include
headers.path = /usr/include/vodman
headers.files = $$HEADERS
INSTALLS += headers




