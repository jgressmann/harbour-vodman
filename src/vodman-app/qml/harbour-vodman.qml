import QtQuick 2.0
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0
import org.duckdns.jgressmann 1.0

import "pages"


ApplicationWindow {
//    initialPage: Component { DownloadPage { } }
//    initialPage: Component { SettingsPage { } }
    initialPage: Component { AboutPage { } }
    cover: Qt.resolvedUrl("cover/CoverPage.qml")
    allowedOrientations: defaultAllowedOrientations

    VodDownloadModel {
        id: vodDownloadModel
    }


    ConfigurationGroup {
        id: settings
        path: "default"
        ConfigurationValue {
            id: defaultFormat
            defaultValue: VM.VM_Any
            key: "/format"
        }

        ConfigurationValue {
            id: defaultDirectory
            key: "/directory"
            defaultValue: StandardPaths.download
        }
    }
}

