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
        MenuItem { text: "largest / best quality" }
        MenuItem { text: "smallest / worst quality" }
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
        console.debug("onCurrentIndexChanged " + currentIndex)
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
        console.debug("propagateFormat " + format)
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
