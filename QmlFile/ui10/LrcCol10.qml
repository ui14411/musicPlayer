import QtQuick
import nusicvideoqml

// 歌词列：双耳页左右各一条（也可单条用）
ListView
{
    id: lrc

    property var model2: []
    property int activeIdx: -1
    property real px: 14.5
    property string fam: Theme.family
    property bool center: true
    property color hiColor: Theme.lyr
    property string emptyText: "无歌词"

    clip: true
    boundsBehavior: Flickable.StopAtBounds
    spacing: 2
    model: lrc.model2

    delegate: Text
    {
        width: lrc.width
        text: modelData.text
        horizontalAlignment: lrc.center ? Text.AlignHCenter : Text.AlignLeft
        color: index === lrc.activeIdx ? lrc.hiColor : Theme.tx2
        opacity: index === lrc.activeIdx ? 1 : 0.42
        font.pixelSize: lrc.px * (index === lrc.activeIdx ? 1.1 : 1)
        font.bold: index === lrc.activeIdx
        font.family: lrc.fam
        elide: Text.ElideRight

        Behavior on opacity { NumberAnimation { duration: 160 } }
    }

    Text
    {
        visible: lrc.count === 0
        anchors.centerIn: parent
        text: lrc.emptyText
        color: Theme.tx3
        font.pixelSize: 13
        font.family: lrc.fam
    }

    onActiveIdxChanged:
    {
        if (activeIdx >= 0)
            positionViewAtIndex(activeIdx, ListView.Center)
    }
}
