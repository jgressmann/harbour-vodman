import QtQuick 2.0
import Sailfish.Silica 1.0


Item {
    id: root

    property alias overlayOpacity: overlay.opacity
    property real progress: 0.314
    property alias color: overlay.color
    property bool inverse: false
    readonly property real _progress: Math.max(0, Math.min(progress, 1))


    Rectangle {
        id: overlay
        height: parent.height
        color: Theme.highlightColor
        opacity: Theme.highlightBackgroundOpacity
        x: {
            if (Qt.LeftToRight === Qt.application.layoutDirection) {
                if (inverse) {
                    return parent.width * _progress
                }

                return 0
            } else {
                if (inverse) {
                    return 0
                }
                return parent.width * (1 - _progress)
            }
        }
        width: inverse ? parent.width * (1 - _progress) : parent.width * _progress
//        Component.onCompleted: {
//            console.debug("overlay w=" + width + " h=" + height)
//        }
    }

//    onProgressChanged: {
//        console.debug("progress=" + progress)

//    }

//    Component.onCompleted: {
//        console.debug("component w=" + width)
//        //_moveToTop()
//    }

    onVisibleChildrenChanged: {
//        console.debug("visible children changed")
        _moveToTop()
    }

    function _moveToTop() {
        var maxZ = 0
        var vc = visibleChildren
//        console.debug("#" + vc.length + " children")
        for (var i = 0; i < vc.length; ++i) {
            var child = vc[i]
            if (child !== overlay) {
                maxZ = Math.max(maxZ, child.z)
            }
        }
//        console.debug("max z " + maxZ)
        overlay.z = maxZ + 1
    }
}

