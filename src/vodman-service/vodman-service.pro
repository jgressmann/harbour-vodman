TEMPLATE = app
TARGET = vodman-service

QT -= gui
QT += dbus

CONFIG += console

include(../../common.pri)



ytdl.extra = cp $$PWD/../../build-ytdl/youtube-dl $$PWD/vodman-youtube-dl
ytdl.target = vodman-youtube-dl
ytdl.files = vodman-youtube-dl
ytdl.path = /usr/bin
INSTALLS += ytdl



DEFINES += VODMAN_YOUTUBEDL_PATH=\"\\\"\"/usr/bin/vodman-youtube-dl\"\\\"\"

target.path = /usr/bin
#target.path = $$DEPLOYMENT_PATH/bin
INSTALLS += target


# generate adaptor code
DBUS_ADAPTORS += ../../dbus/org.duckdns.jgressmann.vodman.service.xml


SERVICE_FILE = ../../dbus/org.duckdns.jgressmann.vodman.service.service
OTHER_FILES +=  $$SERVICE_FILE
service.files = $$SERVICE_FILE
service.path  = /usr/share/dbus-1/services
# don't install until we according to harbour policy
INSTALLS += service

interface.path = /usr/share/dbus-1/interfaces
interface.files = ../../dbus/org.duckdns.jgressmann.vodman.service.xml
INSTALLS += interface

DISTFILES += $$SERVICE_FILE

HEADERS += \
    $$PWD/VMService.h \
    $$PWD/VMYTDL.h

SOURCES += $$PWD/vodman-service.cpp \
    $$PWD/VMService.cpp \
    $$PWD/VMYTDL.cpp


#QMAKE_RPATHDIR += $${DEPLOYMENT_PATH}/lib
LIBS += -L../vodman-lib -lvodman
