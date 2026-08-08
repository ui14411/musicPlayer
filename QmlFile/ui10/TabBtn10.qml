import QtQuick
import nusicvideoqml

// 首页三个 tab 的按钮（横排 / 竖排 / 胶囊三种形态）
Rectangle
{
    id: tabBtn

    property string label: ""
    property bool active: false
    property bool vertical: false
    property bool pills: false
    property bool hovered: false

    signal clicked()

    width: vertical ? 56 : (pills ? lbl.implicitWidth + 26 : Math.max(96, lbl.implicitWidth + 34))
    height: vertical ? 92 : 34
    radius: pills ? height / 2 : (active && !pills ? 10 : Theme.r1)
    color: active ? (pills ? Theme.acc : Theme.sel) : (hovered ? Theme.hov : "transparent")
    border.color: pills && active ? "transparent" : (hovered ? Theme.brd : "transparent")
    border.width: 1

    Text
    {
        id: lbl
        anchors.centerIn: parent
        text: tabBtn.label
        rotation: tabBtn.vertical ? 90 : 0
        color: tabBtn.active ? (tabBtn.pills ? Theme.acctx : Theme.acc) : Theme.tx2
        font.pixelSize: 13
        font.bold: true
        font.family: Theme.family
    }

    // 选中下划线（贴面板的那种 tab）
    Rectangle
    {
        visible: tabBtn.active && !tabBtn.pills && !tabBtn.vertical
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 2
        width: parent.width - 20
        height: 2
        radius: 1
        color: Theme.acc
    }

    MouseArea
    {
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onEntered: tabBtn.hovered = true
        onExited: tabBtn.hovered = false
        onClicked: tabBtn.clicked()
    }
}
