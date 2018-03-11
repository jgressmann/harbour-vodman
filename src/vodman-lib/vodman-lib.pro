TEMPLATE = lib
VERSION = 1.0.0
TARGET = vodman

QT -= gui
QT += core dbus


CONFIG += console
CONFIG -= app_bundle

target.path = /usr/lib
INSTALLS += target



include(../../common.pri)

SOURCES += \
    VMVod.cpp \
    misc.cpp \
    VMVodFileDownload.cpp \
    VMVodMetaDataDownload.cpp \

HEADERS += \
    VMVod.h \
    VMVodFileDownload.h \
    VMVodMetaDataDownload.h \
    misc.h \


headers.path = /usr/include/vodman
headers.files = $$HEADERS
INSTALLS += headers


