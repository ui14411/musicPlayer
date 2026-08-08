import QtQuick
import Qt5Compat.GraphicalEffects
import nusicvideoqml

// 封面 + 两行文字（底栏用，也可单独摆）
Item
{
    id: info

    property real coverPx: 48
    property int coverRadius: 999
    property string cover: ""
    property string title: ""
    property string sub: ""
    property bool showText: true
    property real textW: 190
    property real titlePx: 13.5
    property real subPx: 11.5
    property color textColor: Theme.tx
    property color subColor: Theme.tx3
    property string fam: Theme.family
    property bool bold: true

    width: showText ? coverPx + 12 + textW : coverPx
    height: coverPx

    Rectangle
    {
        id: infoCvBg
        width: info.coverPx
        height: info.coverPx
        radius: Math.min(info.coverRadius, width / 2)
        color: Theme.pan2
        border.color: Theme.brd
        border.width: 1
        anchors.verticalCenter: parent.verticalCenter

        Image
        {
            id: infoCv
            anchors.fill: parent
            anchors.margins: 1
            source: info.cover
            fillMode: Image.PreserveAspectCrop
            smooth: true
            layer.enabled: true
            layer.effect: OpacityMask
            {
                maskSource: Rectangle
                {
                    width: infoCv.width
                    height: infoCv.height
                    radius: Math.min(info.coverRadius, width / 2)
                    color: "white"
                }
            }
        }
    }

    Column
    {
        visible: info.showText
        anchors.left: infoCvBg.right
        anchors.leftMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        spacing: 3

        Text
        {
            width: info.textW
            text: info.title
            color: info.textColor
            font.pixelSize: info.titlePx
            font.bold: info.bold
            font.family: info.fam
            elide: Text.ElideRight
        }

        Text
        {
            width: info.textW
            text: info.sub
            color: info.subColor
            font.pixelSize: info.subPx
            font.family: info.fam
            elide: Text.ElideRight
        }
    }
}
