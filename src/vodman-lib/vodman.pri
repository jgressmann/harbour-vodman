VODMAN_LIB_NAMESPACE=Vodman
DEFINES += VODMAN_LIB_VERSION_MAJOR=2
DEFINES += VODMAN_LIB_VERSION_MINOR=0
DEFINES += VODMAN_LIB_VERSION_PATCH=0
DEFINES += VODMAN_LIB_NAMESPACE=\"\\\"\"$$VODMAN_LIB_NAMESPACE\"\\\"\"


isEmpty(VODMAN_LIB_TARGETDIR) {
    VODMAN_LIB_TARGETDIR = $$_PRO_FILE_PWD_
}


SOURCES += \
    $$PWD/VMYTDL.cpp \
    $$PWD/VMQuickYTDLDownloader.cpp \
    $$PWD/VMPlaylistDownload.cpp \
    $$PWD/VMMetaDataDownload.cpp \
    $$PWD/VMPlaylist.cpp

HEADERS += \
    $$PWD/VMYTDL.h \
    $$PWD/VMQuickYTDLDownloader.h \
    $$PWD/VMMetaDataDownload.h \
    $$PWD/VMPlaylistDownload.h \
    $$PWD/VMPlaylist.h



DISTFILES += $$PWD/qml/pages/YTDLPage.qml
OTHER_FILES += $$PWD/config.json


vodmanfiles.commands += $(COPY_DIR) $$shell_path($$PWD/qml) $$shell_path($$VODMAN_LIB_TARGETDIR);

PRE_TARGETDEPS += vodmanfiles
QMAKE_EXTRA_TARGETS += vodmanfiles
INCLUDEPATH += $$PWD


