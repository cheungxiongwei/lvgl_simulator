import QtQuick
import LVGLQML

Window {
  title: qsTr("LVGLQML")
  width: 640
  height: 480
  visible: true

  Lvgl {
    anchors.fill: parent
  }
}
