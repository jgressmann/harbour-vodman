import QtQuick 2.0
import Sailfish.Silica 1.0
import org.duckdns.jgressmann 1.0
import ".."

Dialog {
    id: root
    property alias format: formatComboBox.format
    canAccept: format !== VM.VM_Any

    Column {
        width: parent.width

        DialogHeader {
            title: "Select a format"
        }

        Flickable {
            // ComboBox requires a flickable ancestor
            width: parent.width
            height: parent.height
            interactive: false

            FormatComboBox {
                id: formatComboBox
                excludeAskEveryTime: true
            }
        }
    }
}
