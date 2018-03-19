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
import Nemo.Configuration 1.0
import org.duckdns.jgressmann 1.0


import "pages"


ApplicationWindow {
    id: root
    initialPage: Component { DownloadPage { } }
//    initialPage: Component { SettingsPage { } }
    cover: Qt.resolvedUrl("cover/CoverPage.qml")

    allowedOrientations: defaultAllowedOrientations

    VodDownloadModel {
        id: vodDownloadModel
    }

    ConfigurationGroup {
        id: settings
        path: "default"

        ConfigurationValue {
            id: settingDefaultFormat
            defaultValue: VM.VM_Any
            key: "/format"
        }

        ConfigurationValue {
            id: settingDefaultDirectory
            key: "/directory"
            defaultValue: StandardPaths.videos
        }

        ConfigurationValue {
            id: settingDefaultFileName
            key: "/fileName"
            defaultValue: "{title}"
        }
    }

    ConfigurationValue {
        id: debugApp
        key: "/debug"
        defaultValue: false
    }
}

