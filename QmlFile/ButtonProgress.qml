import QtQuick
import QtQuick.Controls
import nusicvideoqml

// 底栏：10 种形态（c1 - c10），外加切套用的色标和数字键
Item
{
    id: dockRoot

    property bool showSwitcher: true
    readonly property int v: Theme.c
    readonly property var g: geo()
    property var stack
    property bool schemeReady: false

    width: parent ? parent.width : 1280
    height: g.ih + (v === 1 || v === 3 || v === 7 || v === 10 ? 16 : 0)
    z: 100

    Component.onCompleted:
    {
        dockRoot.stack = StackView.view
        // 上次选的那套存在他已有的 config/setting.ini 里（PlayMusic 的 QSettings，键名 uiScheme）
        if (player.uiScheme >= 1 && player.uiScheme <= Theme.schemes.length)
            Theme.scheme = player.uiScheme
        dockRoot.schemeReady = true
    }

    Connections
    {
        target: Theme

        function onSchemeChanged()
        {
            if (dockRoot.schemeReady)
                player.uiScheme = Theme.scheme
        }
    }

    // 点底栏左半（封面 + 曲名）= 进/出播放页，和他 ButtonProgress 的行为一致
    MouseArea
    {
        id: dockTap
        x: infoL.x
        y: bg.y
        width: Math.min(infoL.width + 12, dockRoot.width - x)
        height: bg.height
        visible: !g.twin && g.progMode !== "top"
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        propagateComposedEvents: false

        onClicked:
        {
            var cur = dockRoot.stack ? dockRoot.stack.currentItem : null
            var on = cur && (cur.objectName === "playPage" || cur.objectName === "doublePage")
            if (on)
            {
                dockRoot.stack.pop()
                player.stopPreview()
            }
            else
            {
                dockRoot.stack.push("Play.qml")
                player.playVideo(player.currentMusicName)
            }
        }
    }

    function geo()
    {
        var W = dockRoot.width
        var o = {
            twin: false, opaque: true, grad: false, topLine: false, progMode: "row",
            iw: Math.min(1060, W * 0.92), ih: Theme.dockH, ix: -1, ir: 999,
            padL: 20, coverPx: 48, coverR: 999, showModel: true, raised: false,
            mono: false, fam: Theme.family
        }

        switch (v)
        {
        case 1: // 悬浮玻璃胶囊
            o.iw = Math.min(1060, W * 0.92); o.ih = 74; o.ir = 37
            o.coverPx = 48; o.coverR = 999
            break
        case 2: // 全宽实心
            o.iw = W; o.ih = 90; o.ir = 0; o.topLine = true
            o.coverPx = 58; o.coverR = Theme.r1
            break
        case 3: // 轻量浮条
            o.iw = Math.min(980, W * 0.9); o.ih = 66; o.ir = Theme.r3
            o.coverPx = 44; o.coverR = Theme.r1
            break
        case 4: // 无边细线
            o.iw = W; o.ih = 90; o.ir = 0; o.opaque = false; o.topLine = true
            o.coverPx = 58; o.coverR = 6
            break
        case 5: // 复古凸起按钮
            o.iw = W; o.ih = 90; o.ir = 0; o.topLine = true; o.raised = true
            o.coverPx = 58; o.coverR = 6
            break
        case 6: // 进度成顶线
            o.iw = W; o.ih = 104; o.ir = 0; o.topLine = true; o.progMode = "top"
            o.coverPx = 58; o.coverR = Theme.r1
            break
        case 7: // 右下紧凑胶囊
            o.iw = Math.min(620, W * 0.6); o.ih = 60; o.ir = 30; o.ix = 24
            o.coverPx = 40; o.coverR = 999; o.showModel = false; o.padL = 14
            break
        case 8: // 渐变融入视频
            o.iw = W; o.ih = 104; o.ir = 0; o.grad = true
            o.coverPx = 58; o.coverR = 999
            break
        case 9: // 终端状态栏
            o.iw = W; o.ih = 64; o.ir = 0; o.topLine = true; o.mono = true
            o.fam = Theme.mono; o.coverPx = 38; o.coverR = 4
            break
        default: // 10 双岛
            o.twin = true; o.iw = W; o.ih = 70; o.ix = 16; o.ir = 35
            o.coverPx = 46; o.coverR = 999; o.progMode = "bottom"
            break
        }
        return o
    }

    Gradient
    {
        id: dockGrad
        GradientStop { position: 0.0; color: "transparent" }
        GradientStop { position: 1.0; color: Theme.dockSol }
    }

    // 单岛 / 全宽底
    Rectangle
    {
        id: bg
        visible: !g.twin
        width: g.iw
        height: g.ih
        x: (g.ix >= 0 && g.iw < dockRoot.width) ? dockRoot.width - g.iw - g.ix
                                                : (dockRoot.width - g.iw) / 2
        y: dockRoot.height - height
        radius: g.ir
        color: g.grad ? "transparent" : (g.opaque ? Theme.dockSol : "transparent")
        gradient: g.grad ? dockGrad : null
        border.color: (g.topLine || g.opaque) ? Theme.brd : "transparent"
        border.width: (g.topLine || g.opaque) ? 1 : 0
    }

    Rectangle
    {
        id: islandL
        visible: g.twin
        y: dockRoot.height - g.ih
        height: g.ih
        radius: g.ir
        x: g.ix
        width: infoL.width + 40
        color: Theme.dockSol
        border.color: Theme.brd
        border.width: 1
    }

    Rectangle
    {
        id: islandR
        visible: g.twin
        y: dockRoot.height - g.ih
        height: g.ih
        radius: g.ir
        x: dockRoot.width - width - g.ix
        width: transR.width + 36
        color: Theme.dockSol
        border.color: Theme.brd
        border.width: 1
    }

    Info10
    {
        id: infoL
        coverPx: g.coverPx
        coverRadius: g.coverR
        cover: Theme.url(player.currentMusicCover)
        title: player.currentMusicName
        sub: player.currentMusicSinger
        fam: g.mono ? Theme.mono : Theme.family
        titlePx: g.mono ? 12 : 13.5
        subPx: g.mono ? 10.5 : 11.5
        textW: g.twin ? 170 : (v === 7 ? 130 : 190)
        x: g.twin ? islandL.x + 20 : bg.x + g.padL
        anchors.verticalCenter: bg.verticalCenter
        anchors.verticalCenterOffset: g.progMode === "top" ? 14 : 0
    }

    Transport10
    {
        id: transR
        btnPx: g.ih > 80 ? 34 : 30
        bigPx: g.ih > 80 ? 42 : 36
        shape: g.coverR
        raised: g.raised
        showModel: g.showModel
        fam: g.mono ? Theme.mono : Theme.family
        x: g.twin ? islandR.x + (islandR.width - width) / 2
                  : infoL.x + infoL.width + 18
        anchors.verticalCenter: bg.verticalCenter
        anchors.verticalCenterOffset: g.progMode === "top" ? 14 : 0
    }

    Progress10
    {
        id: progRow
        visible: g.progMode === "row"
        x: transR.x + transR.width + 20
        anchors.verticalCenter: bg.verticalCenter
        fam: g.mono ? Theme.mono : Theme.family
        timeColor: Theme.tx3
        barW: Math.max(120, bg.x + bg.width - g.padL - x)
    }

    // c6：进度是底栏顶边那条 3px 线
    Rectangle
    {
        id: progTop
        visible: g.progMode === "top"
        x: bg.x
        y: bg.y
        width: bg.width
        height: 3
        color: Theme.rail

        Rectangle
        {
            width: progTop.width * (player.musicDuration > 0 ? player.musicPosition / player.musicDuration : 0)
            height: 3
            color: Theme.acc

            Behavior on width
            {
                NumberAnimation { duration: 180 }
            }
        }

        MouseArea
        {
            anchors.fill: parent
            property real frac: Math.min(1, Math.max(0, mouseX / width))
            onPressed: player.setPosition(frac * player.musicDuration)
            onPositionChanged: if (pressed) player.setPosition(frac * player.musicDuration)
        }
    }

    // c10：进度浮在两岛之间
    Progress10
    {
        id: progBottom
        visible: g.progMode === "bottom"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 2
        barW: Math.min(560, dockRoot.width * 0.46)
    }

    // 切套色标（放在底栏上方，不压内容）
    Row
    {
        id: chips
        visible: dockRoot.showSwitcher
        spacing: 4
        anchors.right: parent.right
        anchors.rightMargin: 18
        anchors.bottom: parent.top
        anchors.bottomMargin: 6
        z: 2

        Repeater
        {
            model: Theme.schemes
            delegate: Rectangle
            {
                property bool hovered: false
                width: 20
                height: 20
                radius: 6
                color: Theme.schemes[index].acc
                border.color: Theme.n === index + 1 ? Theme.tx : "#33FFFFFF"
                border.width: Theme.n === index + 1 ? 2 : 1
                opacity: hovered ? 1 : 0.72

                Text
                {
                    anchors.centerIn: parent
                    text: index + 1 === 10 ? "0" : "" + (index + 1)
                    color: Theme.schemes[index].acctx
                    font.pixelSize: 11
                    font.bold: true
                }

                MouseArea
                {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onEntered: hovered = true
                    onExited: hovered = false
                    onClicked: Theme.scheme = index + 1
                }

                ToolTip
                {
                    visible: parent.hovered
                    text: Theme.schemes[index].name
                    delay: 250
                }
            }
        }
    }

    Shortcut { sequence: "1"; onActivated: Theme.scheme = 1 }
    Shortcut { sequence: "2"; onActivated: Theme.scheme = 2 }
    Shortcut { sequence: "3"; onActivated: Theme.scheme = 3 }
    Shortcut { sequence: "4"; onActivated: Theme.scheme = 4 }
    Shortcut { sequence: "5"; onActivated: Theme.scheme = 5 }
    Shortcut { sequence: "6"; onActivated: Theme.scheme = 6 }
    Shortcut { sequence: "7"; onActivated: Theme.scheme = 7 }
    Shortcut { sequence: "8"; onActivated: Theme.scheme = 8 }
    Shortcut { sequence: "9"; onActivated: Theme.scheme = 9 }
    Shortcut { sequence: "0"; onActivated: Theme.scheme = 10 }

    Shortcut
    {
        sequence: "["
        onActivated: Theme.scheme = Theme.n === 1 ? 10 : Theme.n - 1
    }

    Shortcut
    {
        sequence: "]"
        onActivated: Theme.scheme = Theme.n === 10 ? 1 : Theme.n + 1
    }
}
