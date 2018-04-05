/* The MIT License (MIT)
 *
 * Copyright (c) 2018 Jean Gressmann <jean@0x42.de>
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
import org.duckdns.jgressmann 1.0


ComboBox {
    id: root

    property int format: VM.VM_Any
    property bool excludeAskEveryTime: false
    currentIndex: -1
    label: "Format"

    menu: ContextMenu {
        id: menu
        MenuItem { text: "best quality (largest)" }
        MenuItem { text: "worst quality (smallest)" }
        MenuItem { text: "1080" }
        MenuItem { text: "720" }
        MenuItem { text: "360" }
        MenuItem { text: "240" }
        MenuItem {
            id: askEveryTime
            text: "ask every time"
        }
    }

    onExcludeAskEveryTimeChanged: {
        currentIndex = -1
        if (excludeAskEveryTime) {
            askEveryTime.parent = null
        } else {
            askEveryTime.parent = menu
        }
    }

    onCurrentIndexChanged: {
//        console.debug("onCurrentIndexChanged " + currentIndex)
        switch (currentIndex) {
        case 0:
            format = VM.VM_Largest
            break
        case 1:
            format = VM.VM_Smallest
            break
        case 2:
            format = VM.VM_1080p
            break
        case 3:
            format = VM.VM_720p
            break
        case 4:
            format = VM.VM_360p
            break
        case 5:
            format = VM.VM_240p
            break
        default:
            format = VM.VM_Any
            break
        }
    }

    function propagateFormat() {
//        console.debug("propagateFormat " + format)
        switch (format) {
        case VM.VM_Largest:
            currentIndex = 0
            break
        case VM.VM_Smallest:
            currentIndex = 1
            break
        case VM.VM_1080p:
            currentIndex = 2
            break
        case VM.VM_720p:
            currentIndex = 3
            break
        case VM.VM_360p:
            currentIndex = 4
            break
        case VM.VM_240p:
            currentIndex = 5
            break
        default:
            if (excludeAskEveryTime) {
                currentIndex = -1
            } else {
                currentIndex = 6
            }
            break
        }
    }

    onFormatChanged: propagateFormat()
    Component.onCompleted: propagateFormat()
}
