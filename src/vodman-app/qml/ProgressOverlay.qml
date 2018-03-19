/* The MIT License (MIT)
 *
 * Copyright (c) 2016 Jean Gressmann <jean@0x42.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

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

