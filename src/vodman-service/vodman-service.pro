TEMPLATE = app
TARGET = vodman-service

QT += dbus
QT -= gui
CONFIG += console



DEFINES += VODMAN_YOUTUBEDL_PATH=\"\\\"\"$${DEPLOYMENT_PATH}/bin/youtube-dl\"\\\"\"

target.path = /usr/share/harbour-vodman/bin
INSTALLS += target


# generate adaptor code
DBUS_ADAPTORS += ../../dbus/org.duckdns.jgressmann.vodman.service.xml

## generate adaptor code
#DBUS_ADAPTORS += vodman
#vodman.files = ../../dbus/org.duckdns.jgressmann.vodman.service.xml
#vodman.header_flags = -i VMService.h -l VMService -c VMServiceAdaptor
#vodman.source_flags = -l VMService -c VMServiceAdaptor

SERVICE_FILE = ../../dbus/org.duckdns.jgressmann.vodman.service.service
OTHER_FILES +=  $$SERVICE_FILE
service.files = $$SERVICE_FILE
service.path  = /usr/share/dbus-1/services/
INSTALLS += service

DISTFILES += $$SERVICE_FILE

HEADERS += \
    $$PWD/VMService.h \
    $$PWD/VMYTDL.h

SOURCES += $$PWD/vodman-service.cpp \
    $$PWD/VMService.cpp \
    $$PWD/VMYTDL.cpp


include(../../common.pri)
LIBS += -L../vodman-lib -lvodman
