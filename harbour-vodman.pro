TEMPLATE = subdirs
SUBDIRS  = src \
    #3rd-party

#include(3rd-party/youtube_dl.pri)

#include(3rd-party/ytplayer.pri)


DISTFILES += COPYING
DISTFILES += COPYING.youtube-dl
DISTFILES += rpm/harbour-vodman.spec
DISTFILES += dbus/org.duckdns.jgressmann.vodman.service.xml

copying.files = COPYING*
copying.path = /usr/share/$${TARGET}
INSTALLS += copying

#ytdl.target = youtube-dl
#ytdl.files = $$PWD/build-ytdl/youtube-dl
##ytdl.path = /usr/share/$${TARGET}/bin
#ytdl.path = /usr/bin
#QMAKE_EXTRA_TARGETS += ytdl
#INSTALLS += ytdl


#desktop.files = harbour-vodman.desktop
#desktop.path = /usr/share/applications
#INSTALLS += desktop

#qml.path = /usr/share/$${TARGET}/qml
#qml.files = qml/*
#INSTALLS += qml

#copytarget.path    = /path/to/installation
#copytarget.files  += $$files(example/filename*)

#icons.files = $$PWD/icons/*/$${TARGET}.png
#icons.path = /usr/share/$${TARGET}/bin
#QMAKE_EXTRA_TARGETS += icons
#INSTALLS += icons


#src.depends = lib
#tests.depends = lib
#declarative.depends = lib

#include(doc/doc.pri)

#systemd.files = transferengine.service
#systemd.path  = /usr/lib/systemd/user/

#OTHER_FILES += \
#    rpm/*.spec \
#    nemo-transfer-engine.conf \
#    dbus/* \
#    doc/src/* \
#    doc/config/*

#INSTALLS += systemd

