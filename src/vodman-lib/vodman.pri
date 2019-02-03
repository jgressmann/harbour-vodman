
SOURCES += \
    $$PWD/VMVod.cpp \
    $$PWD/VMVodFileDownload.cpp \
    $$PWD/VMVodMetaDataDownload.cpp \
    $$PWD/VMService.cpp \
    $$PWD/VMYTDL.cpp \
    $$PWD/VMQuickYTDLDownloader.cpp

HEADERS += \
    $$PWD/VMVod.h \
    $$PWD/VMVodFileDownload.h \
    $$PWD/VMVodMetaDataDownload.h \
    $$PWD/VMService.h \
    $$PWD/VMYTDL.h \
    $$PWD/VMQuickYTDLDownloader.h



DISTFILES += $$PWD/qml/pages/YTDLPage.qml
OTHER_FILES += $$PWD/config.json


vodmanfiles.commands += $(COPY_DIR) $$shell_path($$PWD/qml) $$shell_path($$_PRO_FILE_PWD_);

PRE_TARGETDEPS += vodmanfiles
QMAKE_EXTRA_TARGETS += vodmanfiles
INCLUDEPATH += $$PWD


