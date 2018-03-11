TEMPLATE = app
TARGET = vodman-cli

QT -= gui
QT += dbus
CONFIG += console

SOURCES += vodman-cli.cpp

target.path = /usr/share/harbour-vodman/bin
INSTALLS += target


DBUS_INTERFACES += ../../dbus/org.duckdns.jgressmann.vodman.xml

include(../../common.pri)
LIBS += -L../vodman-lib -lvodman
