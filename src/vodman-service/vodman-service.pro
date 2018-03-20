TEMPLATE = app
TARGET = vodman-service

QT += dbus
QT -= gui
CONFIG += console

include(../../common.pri)

#copyYoutubeDl.files = ../../build-ytdl/youtube-dl
#copyYoutubeDl.extra = cp $$copyYoutubeDl.files $$OUT_PWD/vodman-youtube-dl
copyYoutubeDl.target = $$OUT_PWD/vodman-youtube-dl
copyYoutubeDl.commands = @echo Hallo

#    echo Hallo ; \
#    cp $$PWD/../../build-ytdl/youtube-dl $$copyYoutubeDl.target
#QMAKE_EXTRA_TARGETS += copyYoutubeDl

ytdl.extra = cp $$PWD/../../build-ytdl/youtube-dl $$PWD/vodman-youtube-dl
ytdl.target = vodman-youtube-dl
ytdl.files = vodman-youtube-dl
#ytdl.path = /usr/share/$${TARGET}/bin
ytdl.path = /usr/bin

#QMAKE_EXTRA_TARGETS += ytdl
INSTALLS += ytdl

#extra_install.path = /usr/bin
#extra_install.files = youtube-dl
#extra_install.
#INSTALLS += extra_install

DEFINES += VODMAN_YOUTUBEDL_PATH=\"\\\"\"/usr/bin/vodman-youtube-dl\"\\\"\"

target.path = /usr/bin
#target.path = $$DEPLOYMENT_PATH/bin
INSTALLS += target


# generate adaptor code
DBUS_ADAPTORS += ../../dbus/org.duckdns.jgressmann.vodman.service.xml


SERVICE_FILE = ../../dbus/org.duckdns.jgressmann.vodman.service.service
OTHER_FILES +=  $$SERVICE_FILE
service.files = $$SERVICE_FILE
service.path  = /usr/share/dbus-1/services/
# don't install until we according to harbour policy
INSTALLS += service

DISTFILES += $$SERVICE_FILE

HEADERS += \
    $$PWD/VMService.h \
    $$PWD/VMYTDL.h

SOURCES += $$PWD/vodman-service.cpp \
    $$PWD/VMService.cpp \
    $$PWD/VMYTDL.cpp


QMAKE_RPATHDIR += $${DEPLOYMENT_PATH}/lib
LIBS += -L../vodman-lib -lvodman
