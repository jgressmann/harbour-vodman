# NOTICE:
#
# Application name defined in TARGET has a corresponding QML filename.
# If name defined in TARGET is changed, the following needs to be done
# to match new name:
#   - corresponding QML filename must be changed
#   - desktop icon filename must be changed
#   - desktop filename must be changed
#   - icon definition filename in desktop file must be changed
#   - translation filenames have to be changed

# The name of your application
TARGET = harbour-vodman

#QT *= dbus
#QT -= gui network

!CONFIG(debug, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT

CONFIG += sailfishapp

DISTFILES += harbour-vodman.desktop
DISTFILES += icons/cover.svg icons/icon.svg icons/tape.png
DISTFILES += qml/harbour-vodman.qml \
    qml/cover/CoverPage.qml \
    qml/pages/DownloadPage.qml \
    qml/pages/SelectFormatDialog.qml \
    qml/pages/SettingsPage.qml \
    qml/pages/AboutPage.qml \
    qml/FormatComboBox.qml \
    qml/ProgressOverlay.qml \
    qml/Global.qml \
    qml/qmldir \
    translations/harbour-vodman.ts \
    translations/harbour-vodman-de.ts \



SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172 256x256

# to disable building translations every time, comment out the
# following CONFIG line
CONFIG += sailfishapp_i18n
CONFIG += sailfishapp_i18n_idbased
CONFIG += sailfishapp_i18n_unfinished

# German translation is enabled as an example. If you aren't
# planning to localize your app, remember to comment out the
# following TRANSLATIONS line. And also do not forget to
# modify the localized app name in the the .desktop file.
TRANSLATIONS += translations/harbour-vodman-de.ts
TRANSLATIONS += translations/harbour-vodman.ts



SOURCES += harbour-vodman.cpp
SOURCES += VMQuickVodDownloadModel.cpp
SOURCES += VMQuickVodDownload.cpp
SOURCES += VMApp.cpp



HEADERS += VMQuickVodDownloadModel.h
HEADERS += VMQuickVodDownload.h
HEADERS += VMApp.h




icons.path = /usr/share/$${TARGET}/icons
icons.files = icons/cover.svg icons/icon.svg icons/tape.png
INSTALLS += icons



include(../common.pri)
include(../vodman-lib/vodman-lib.pri)



