# known to qmake
VER_MAJ = 1
VER_MIN = 1
VER_PAT = 2

VERSION = $${VER_MAJ}.$${VER_MIN}.$${VER_PAT}

VODMAN_NAMESPACE=org.duckdns.jgressmann
DEFINES += VODMAN_VERSION_MAJOR=$$VER_MAJ
DEFINES += VODMAN_VERSION_MINOR=$$VER_MIN
DEFINES += VODMAN_VERSION_PATCH=$$VER_PAT
DEFINES += VODMAN_NAMESPACE=\"\\\"\"$$VODMAN_NAMESPACE\"\\\"\"



