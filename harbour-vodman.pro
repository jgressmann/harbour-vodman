TEMPLATE = subdirs
SUBDIRS  = src

DISTFILES += COPYING
DISTFILES += COPYING.youtube-dl
DISTFILES += rpm/harbour-vodman.spec

copying.files = COPYING*
copying.path = /usr/share/$${TARGET}
INSTALLS += copying

