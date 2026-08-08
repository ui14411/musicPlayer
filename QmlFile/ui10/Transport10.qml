import QtQuick
import QtQuick.Controls
import nusicvideoqml

// 上一曲 / 播放暂停 / 下一曲 / 播放模式 / 人声伴奏模式
Item
{
    id: transport

    property real btnPx: 34
    property real bigPx: 42
    property int shape: 999
    property bool raised: false
    property bool showModel: true
    property string fam: Theme.family
    property color iconColor: Theme.tx2
    property color accentBg: Theme.acc
    property color accentFg: Theme.acctx
    property int gap: 6

    width: row.implicitWidth
    height: bigPx

    function patternText(v)
    {
        if (v === 0) return "原声"
        if (v === 1) return "人声"
        if (v === 2) return "伴奏"
        if (v === 3) return "环绕"
        return "模式"
    }

    function modelText(v)
    {
        if (v === 2) return "顺序"
        if (v === 1) return "随机"
        return "单曲"
    }

    Row
    {
        id: row
        anchors.verticalCenter: parent.verticalCenter
        spacing: transport.gap

        Repeater
        {
            model: [
                { "t": "⏮", "big": false, "act": "prev" },
                { "t": player && player.playing ? "⏸" : "▶", "big": true, "act": "toggle" },
                { "t": "⏭", "big": false, "act": "next" }
            ]
            delegate: Rectangle
            {
                id: tBtn

                property bool hovered: false
                property bool isBig: modelData.big
                width: isBig ? transport.bigPx : transport.btnPx
                height: width
                radius: isBig ? width / 2 : Math.min(transport.shape, width / 2)
                color: isBig ? transport.accentBg
                     : (hovered ? Theme.hov : (transport.raised ? Theme.pan2 : "transparent"))
                border.color: transport.raised ? Theme.brd : "transparent"
                border.width: transport.raised ? 1 : 0

                Text
                {
                    anchors.centerIn: parent
                    text: modelData.t
                    color: tBtn.isBig ? transport.accentFg : transport.iconColor
                    font.pixelSize: tBtn.isBig ? transport.bigPx * 0.42 : transport.btnPx * 0.44
                    font.family: transport.fam
                }

                MouseArea
                {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onEntered: tBtn.hovered = true
                    onExited: tBtn.hovered = false
                    onClicked:
                    {
                        if (modelData.act === "prev")
                            player.prevMusic()
                        else if (modelData.act === "next")
                            player.nextMusic()
                        else if (player.playing)
                            player.Pause()
                        else
                            player.Play()
                    }
                }
            }
        }

        // 播放模式
        Rectangle
        {
            id: modelBtn

            visible: transport.showModel
            property bool hovered: false
            height: transport.btnPx
            width: mTxt.implicitWidth + 18
            radius: Math.min(transport.shape, height / 2)
            color: hovered ? Theme.hov : "transparent"

            Text
            {
                id: mTxt
                anchors.centerIn: parent
                text: transport.modelText(player.playmodel) + "播放"
                color: transport.iconColor
                font.pixelSize: 11
                font.family: transport.fam
            }

            MouseArea
            {
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onEntered: modelBtn.hovered = true
                onExited: modelBtn.hovered = false
                onClicked: player.switchModel()
            }
        }

        // 人声 / 伴奏 / 环绕
        Rectangle
        {
            id: patBtn

            property bool hovered: false
            height: transport.btnPx
            width: pTxt.implicitWidth + 20
            radius: Math.min(transport.shape, height / 2)
            color: hovered ? Theme.hov : "transparent"
            border.color: hovered ? Theme.acc : Theme.brd
            border.width: 1

            Text
            {
                id: pTxt
                anchors.centerIn: parent
                text: transport.patternText(player.playpattern) + " ▼"
                color: transport.iconColor
                font.pixelSize: 12
                font.family: transport.fam
            }

            MouseArea
            {
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onEntered: patBtn.hovered = true
                onExited: patBtn.hovered = false
                onClicked: patMenu.visible ? patMenu.close() : patMenu.open()
            }

            Popup
            {
                id: patMenu

                parent: Overlay.overlay
                x: Math.max(8, Math.min(patBtn.mapToGlobal(0, 0).x, (parent ? parent.width : 1280) - width - 8))
                y: Math.max(8, patBtn.mapToGlobal(0, 0).y - height - 8)
                width: 150
                height: 4 * 40 + 12
                padding: 6
                modal: false
                closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

                background: Rectangle
                {
                    radius: Theme.r1
                    color: Theme.pan
                    border.color: Theme.brd
                    border.width: 1
                }

                Column
                {
                    anchors.fill: parent
                    spacing: 2

                    Repeater
                    {
                        model: ["原声", "人声", "伴奏", "环绕"]
                        delegate: Rectangle
                        {
                            property bool hovered: false
                            width: parent.width
                            height: 36
                            radius: 8
                            color: hovered ? Theme.hov
                                   : (index === player.playpattern ? Theme.sel : "transparent")

                            Text
                            {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left
                                anchors.leftMargin: 12
                                text: (index === player.playpattern ? "✓ " : "") + modelData
                                color: Theme.tx
                                font.pixelSize: 13
                                font.family: transport.fam
                            }

                            MouseArea
                            {
                                anchors.fill: parent
                                hoverEnabled: true
                                onEntered: parent.hovered = true
                                onExited: parent.hovered = false
                                onClicked:
                                {
                                    player.playpattern = index
                                    patMenu.close()
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
