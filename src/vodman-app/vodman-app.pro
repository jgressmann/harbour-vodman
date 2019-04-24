# known to qmake
VER_MAJ = 1
VER_MIN = 2
VER_PAT = 0

VERSION = $${VER_MAJ}.$${VER_MIN}.$${VER_PAT}


VODMAN_NAMESPACE=org.duckdns.jgressmann
DEFINES += VODMAN_VERSION_MAJOR=$$VER_MAJ
DEFINES += VODMAN_VERSION_MINOR=$$VER_MIN
DEFINES += VODMAN_VERSION_PATCH=$$VER_PAT
DEFINES += VODMAN_NAMESPACE=\"\\\"\"$$VODMAN_NAMESPACE\"\\\"\"

include(../vodman-lib/vodman.pri)

TARGET = harbour-vodman


!CONFIG(debug, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT

DEFINES += VODMAN_APP_NAME=\"\\\"\"harbour-vodman\"\\\"\"

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
    translations/harbour-vodman-sv.ts \
    translations/harbour-vodman-zh_cn.ts \


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
TRANSLATIONS += translations/harbour-vodman.ts
TRANSLATIONS += translations/harbour-vodman-de.ts
TRANSLATIONS += translations/harbour-vodman-sv.ts
TRANSLATIONS += translations/harbour-vodman-zh_cn.ts




SOURCES += harbour-vodman.cpp \
    VMQuickVodPlaylistDownload.cpp \
    VMQuickVodPlaylistDownloadModel.cpp
SOURCES += VMApp.cpp



HEADERS += \
    VMQuickVodPlaylistDownload.h \
    VMQuickVodPlaylistDownloadModel.h
HEADERS += VMApp.h




icons.path = /usr/share/$${TARGET}/icons
icons.files = icons/cover.svg icons/icon.svg icons/tape.png
INSTALLS += icons

