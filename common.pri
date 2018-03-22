# known to qmake
VER_MAJ = 1
VER_MIN = 0
VER_PAT = 2

VERSION = $${VER_MAJ}.$${VER_MIN}.$${VER_PAT}

#DEPLOYMENT_PATH=/usr/share/harbour-vodman
VODMAN_NAMESPACE=org.duckdns.jgressmann
#DEFINES += VODMAN_DEPLOYMENT_PATH=\"\\\"\"$$DEPLOYMENT_PATH\"\\\"\"
DEFINES += VODMAN_APP_NAME=\"\\\"\"harbour-vodman\"\\\"\"
DEFINES += VODMAN_VERSION_MAJOR=$$VER_MAJ
DEFINES += VODMAN_VERSION_MINOR=$$VER_MIN
DEFINES += VODMAN_VERSION_PATCH=$$VER_PAT
DEFINES += VODMAN_NAMESPACE=\"\\\"\"$$VODMAN_NAMESPACE\"\\\"\"

INCLUDEPATH += $$PWD/src/vodman-lib

