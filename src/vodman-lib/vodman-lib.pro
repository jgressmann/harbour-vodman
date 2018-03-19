TEMPLATE = lib
TARGET = vodman

QT -= gui
QT += core dbus


CONFIG += console
CONFIG -= app_bundle

include(../../common.pri)

VERSION = $$VERSION_MAJOR.$$VERSION_MINOR.$$VERSION_PATCH

target.path = /usr/lib
#target.path = $$DEPLOYMENT_PATH/lib
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


