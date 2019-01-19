
SOURCES += \
    $$PWD/VMVod.cpp \
    $$PWD/VMVodFileDownload.cpp \
    $$PWD/VMVodMetaDataDownload.cpp \
    $$PWD/VMService.cpp \
    $$PWD/VMYTDL.cpp

HEADERS += \
    $$PWD/VMVod.h \
    $$PWD/VMVodFileDownload.h \
    $$PWD/VMVodMetaDataDownload.h \
    $$PWD/VMService.h \
    $$PWD/VMYTDL.h

# Symlink ytdl.qrc & youtube-dl into app directory
RESOURCES += youtube-dl.qrc


