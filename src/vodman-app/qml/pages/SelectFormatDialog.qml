/* The MIT License (MIT)
 *
 * Copyright (c) 2018, 2019 Jean Gressmann <jean@0x42.de>
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


Dialog {
    id: root

    property alias formatIndex: comboBox.currentIndex
    canAccept: formatIndex !== -1
    property var values: []
    property var labels: []

    onLabelsChanged: _updateMenu()
    onValuesChanged: _updateMenu()

    function _updateMenu() {
//        console.debug("currentIndex=" + comboBox.currentIndex)
        comboBox.currentIndex = -1
        if (values && labels && values.length === labels.length) {
//            console.debug("make menu")
//            console.debug("labels=" + labels)
//            console.debug("values=" + values)
            comboBox.menu = _makeMenu()
        } else {
            comboBox.menu = null
        }
    }

    function _makeMenu() {
        var str = "
import QtQuick 2.0
import Sailfish.Silica 1.0

ContextMenu {
"
        for (var i = 0; i < labels.length; ++i) {
            var label = labels[i]
            str += "MenuItem {
text: \"" + label + "\"
}
"
        }

        str += "}\n"

        return Qt.createQmlObject(str, comboBox)
    }

    Column {
        width: parent.width

        DialogHeader {
            title: qsTrId("select-format-dialog-title") // Select a format
        }

        Flickable {
            // ComboBox requires a flickable ancestor
            width: parent.width
            height: parent.height
            interactive: false

            ComboBox {
                id: comboBox
                label: qsTrId("format-label")
            }
        }
    }
}
