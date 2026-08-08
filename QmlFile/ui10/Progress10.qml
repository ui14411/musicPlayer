import QtQuick
import nusicvideoqml

// 当前时间 — 进度条 — 总时长
Item
{
    id: prog

    property real barW: 400
    property real timePx: 11.5
    property real thickness: 4
    property bool showTime: true
    property string fam: Theme.family
    property color timeColor: Theme.tx3
    property bool vertical: false

    width: vertical ? 26 : barW
    height: vertical ? 120 : Math.max(22, thickness + 14)

    Row
    {
        visible: !prog.vertical
        anchors.fill: parent
        spacing: 10

        Text
        {
            visible: prog.showTime
            width: 40
            horizontalAlignment: Text.AlignRight
            anchors.verticalCenter: parent.verticalCenter
            text: Theme.ms(player.musicPosition + player.lyricOffset)
            color: prog.timeColor
            font.pixelSize: prog.timePx
            font.family: prog.fam
        }

        Slide10
        {
            id: pSl
            width: prog.barW - (prog.showTime ? 110 : 20)
            anchors.verticalCenter: parent.verticalCenter
            from: 0
            to: 1
            thickness: prog.thickness
            value: 0

            onMoved: player.setPosition(value * player.musicDuration)
        }

        Text
        {
            visible: prog.showTime
            width: 40
            anchors.verticalCenter: parent.verticalCenter
            text: Theme.ms(player.musicDuration)
            color: prog.timeColor
            font.pixelSize: prog.timePx
            font.family: prog.fam
        }
    }

    Connections
    {
        target: player

        function onMusicPositionChanged()
        {
            if (!pSl.pressed && player.musicDuration > 0)
                pSl.value = player.musicPosition / player.musicDuration
        }

        function onMusicDurationChanged()
        {
            pSl.value = 0
        }
    }
}
