TEMPLATE = app
TARGET = vodman-cli

QT -= gui
QT += dbus
CONFIG += console

DBUS_INTERFACES += ../../dbus/org.duckdns.jgressmann.vodman.service.xml


SOURCES += vodman-cli.cpp

target.path = /usr/bin
#target.path = /usr/share/harbour-vodman/bin
INSTALLS += target



#DBUS_ADAPTORS += $$PWD/../../dbus/org.duckdns.jgressmann.vodman.service.xml


include(../../common.pri)
QMAKE_RPATHDIR += $${DEPLOYMENT_PATH}/lib
LIBS += -L../vodman-lib -lvodman
