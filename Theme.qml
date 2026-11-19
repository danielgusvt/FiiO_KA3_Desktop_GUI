pragma Singleton
import QtQuick

QtObject {
    readonly property color bg: "#161826"
    readonly property color surface: "#232532"
    readonly property color text: "#e9e9ed"
    readonly property color accent: "#9184d9"
    readonly property var n: ({100:"#f3f5fe",200:"#e4e7f5",300:"#cfd3e5",400:"#b2b6ca",500:"#9397ab",
                               600:"#75798c",700:"#595d6c",800:"#3f424d",900:"#292b31"})
    readonly property var a: ({100:"#f5f4ff",200:"#e7e5fe",300:"#d2cefd",400:"#b5abfc",500:"#968ae0",
                               600:"#796cbf",700:"#5d5294",800:"#423a6a",900:"#2b2741"})
    readonly property int radius: 8
    readonly property string font: "Inter"
}
