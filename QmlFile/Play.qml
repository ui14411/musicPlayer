import QtQuick
import QtQuick.Controls
import QtMultimedia
import Qt5Compat.GraphicalEffects
import nusicvideoqml

// 播放页：10 套布局（p1 - p10）
// 想挪位置只改 lay() 里对应那一段的数字，别动下面的控件块
Item
{
    id: playRoot

    property var stack
    objectName: "playPage"

    readonly property int v: Theme.p
    readonly property var g: lay()

    property int currentLrcIndex: -1
    property var bars: []

    Component.onCompleted:
    {
        stack = StackView.view
    }

    // ===== 几何表：每个 case 就是一次布局 =====
    function lay()
    {
        var W = st.width
        var H = st.height
        var o = {
            discS: Math.min(420, W * 0.34, H * 0.62),
            discX: 0, discY: 0,
            infoX: 0, infoY: 0, infoW: Math.min(300, W * 0.24),
            align: "left",
            lrcX: 0, lrcY: 0, lrcW: 0, lrcH: 0,
            lrcPx: Theme.lyricPx, lrcSingle: false, lrcAlign: "left",
            cardX: 0, cardY: 0, cardW: 0, cardH: 0, rw: 0,
            offX: 0, offY: 0, offH: Math.min(300, H * 0.5),
            ctlPx: 36, bigPx: 44,
            card: false, panelR: false, dividers: false, mono: false,
            titlePx: Math.max(20, Math.min(28, H * 0.039)),
            titleFam: Theme.family, volRow: true
        }

        switch (v)
        {
        case 1: // 极光深空：圆盘左 / 信息列中 / 歌词右
            o.discS = Math.min(420, W * 0.34, H * 0.64)
            o.discX = 0; o.discY = (H - o.discS) / 2 - 12
            o.infoX = o.discS + 44; o.infoY = H / 2 - 138
            o.offX = o.infoX + o.infoW + 22; o.offY = H / 2 - o.offH / 2
            o.lrcX = o.offX + 44; o.lrcW = W - o.lrcX
            o.lrcY = 0; o.lrcH = H
            break

        case 2: // 现场红：超大圆盘 + 大字歌词
            o.discS = Math.min(480, W * 0.40, H * 0.78)
            o.discX = 0; o.discY = (H - o.discS) / 2
            o.infoX = o.discS + 40; o.infoY = H / 2 - 120
            o.offX = o.infoX + o.infoW + 18; o.offY = H / 2 - o.offH / 2
            o.lrcX = o.offX + 40; o.lrcW = W - o.lrcX
            o.lrcY = 0; o.lrcH = H
            o.lrcPx = 22
            break

        case 3: // 奶油白：居中一列 + 底部单行词
            o.discS = Math.min(330, W * 0.28, H * 0.42)
            o.discX = (W - o.discS) / 2; o.discY = 0
            o.infoW = Math.min(560, W * 0.7); o.infoX = (W - o.infoW) / 2
            o.infoY = o.discS + 20
            o.align = "center"; o.volRow = false
            o.lrcX = 0; o.lrcW = W; o.lrcY = H - 56; o.lrcH = 52
            o.lrcSingle = true; o.lrcAlign = "center"; o.lrcPx = 18
            o.offX = o.infoX + o.infoW + 20; o.offY = H / 2 - o.offH / 2
            break

        case 4: // 北欧海：镜像——歌词在左，圆盘在右
            o.discS = Math.min(400, W * 0.33, H * 0.66)
            o.discX = W - o.discS; o.discY = (H - o.discS) / 2
            o.infoW = Math.min(280, W * 0.23); o.infoX = o.discX - o.infoW - 46
            o.infoY = H / 2 - 130; o.align = "right"
            o.offX = o.infoX - 42; o.offY = H / 2 - o.offH / 2
            o.lrcX = 0; o.lrcW = o.offX - 44; o.lrcY = 0; o.lrcH = H
            break

        case 5: // 索尼黑：超高歌词居中，小圆盘压右下，信息压左下
            o.discS = Math.min(190, W * 0.16, H * 0.30)
            o.discX = W - o.discS - 8; o.discY = H - o.discS
            o.infoW = Math.min(400, W * 0.3); o.infoX = 0
            o.infoY = H - o.infoW * 0.62; o.align = "left"
            o.volRow = true
            o.lrcW = Math.min(760, W * 0.5); o.lrcX = (W - o.lrcW) / 2
            o.lrcY = 0; o.lrcH = H * 0.62
            o.lrcPx = 24
            o.ctlPx = 30; o.bigPx = 38
            o.offX = o.lrcX + o.lrcW + 22; o.offY = H / 2 - o.offH / 2
            break

        case 6: // 影院金：上带（圆盘 + 信息并排），歌词铺满下面
            o.discS = Math.min(190, W * 0.17, H * 0.30)
            o.discX = 0; o.discY = 0
            o.infoW = Math.min(560, W - o.discS - 60)
            o.infoX = o.discS + 26; o.infoY = 8
            o.titlePx = 30; o.titleFam = Theme.serif
            o.lrcX = 0; o.lrcW = W
            o.lrcY = Math.max(o.discS, 150) + 34; o.lrcH = H - o.lrcY
            o.lrcPx = 16
            o.offX = W - 46; o.offY = 12
            o.offH = Math.min(o.offH, Math.max(60, o.lrcY - 24))
            break

        case 7: // 白空：居中玻璃卡，卡下放歌词
            o.card = true
            o.discS = Math.min(280, W * 0.24, H * 0.40)
            var cw = Math.min(380, W * 0.34)
            var cx = (W - cw) / 2
            var ch = o.discS + 178
            o.discX = cx + (cw - o.discS) / 2; o.discY = 26
            o.infoW = cw - 44; o.infoX = cx + 22; o.infoY = o.discS + 24
            o.align = "center"
            o.lrcW = Math.min(520, W * 0.5); o.lrcX = (W - o.lrcW) / 2
            o.lrcY = ch + 26; o.lrcH = H - o.lrcY
            o.cardX = cx; o.cardY = 0; o.cardW = cw; o.cardH = ch
            o.offX = o.cardX + o.cardW + 22; o.offY = H / 2 - o.offH / 2
            break

        case 8: // 蒸汽波：三段等宽 + 竖分隔线
            o.dividers = true
            var zw = W / 3
            o.discS = Math.min(260, zw * 0.8, H * 0.44)
            o.discX = zw / 2 - o.discS / 2; o.discY = H / 2 - o.discS / 2 - 46
            o.infoW = zw - 44; o.infoX = zw + 22; o.infoY = H / 2 - 124
            o.align = "center"
            o.lrcX = zw * 2 + 26; o.lrcW = zw - 52
            o.lrcY = H / 2 - 170; o.lrcH = 340
            o.lrcPx = 15
            o.offX = zw * 2 - 13; o.offY = H / 2 - o.offH / 2
            break

        case 9: // 杂志纸：巨型标题左上，小封面右上，底部歌词条
            o.discS = Math.min(200, W * 0.17, H * 0.28)
            o.discX = W - o.discS - 40; o.discY = 0
            o.infoW = Math.min(820, W * 0.62); o.infoX = 0; o.infoY = 0
            o.align = "left"
            o.titlePx = Math.max(34, Math.min(56, H * 0.078))
            o.titleFam = Theme.serif
            o.lrcX = 0; o.lrcW = W - o.discS - 90
            o.lrcY = H - 54; o.lrcH = 54; o.lrcPx = 19
            o.offX = o.infoW + 90; o.offY = H / 2 - o.offH / 2
            break

        default: // 10 终端绿：左 2/3 视频叠词，右 360 控制柱
            o.panelR = true; o.mono = true
            var rw = Math.min(360, W * 0.32)
            o.discS = Math.min(250, rw * 0.66, H * 0.34)
            o.discX = W - rw + (rw - o.discS) / 2; o.discY = H * 0.10
            o.infoW = rw - 44; o.infoX = W - rw + 22
            o.infoY = o.discY + o.discS + 22
            o.align = "center"
            o.titleFam = Theme.mono; o.titlePx = 20
            o.lrcX = 40; lrcRightFix(o, W, rw)
            o.lrcY = H - 200; o.lrcH = 180; o.lrcPx = 20
            o.rw = rw
            o.offX = W - o.rw - 38; o.offY = H / 2 - o.offH / 2
            break
        }
        return o
    }

    function lrcRightFix(o, W, rw)
    {
        o.lrcW = W - rw - o.lrcX - 40
    }

    // ===== 背景：真实视频铺满（Play.qml:64-79 那套，音频静音）=====
    AudioOutput
    {
        id: vidAudio
        volume: 0
    }

    VideoOutput
    {
        id: pgOutput
        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectCrop
    }

    MediaPlayer
    {
        id: pgVideo
        videoOutput: pgOutput
        audioOutput: vidAudio
        source: Theme.url(player.videoPath)
        autoPlay: true
        loops: MediaPlayer.Infinite
    }

    Rectangle
    {
        anchors.fill: parent
        color: Theme.veil
        opacity: 0.52
    }

    // ===== 返回 =====
    Rectangle
    {
        id: backBtn
        property bool hovered: false
        anchors.left: parent.left
        anchors.leftMargin: 30
        anchors.top: parent.top
        anchors.topMargin: 22
        width: backTxt.implicitWidth + 28
        height: 34
        radius: height / 2
        color: hovered ? Theme.pan2 : "transparent"
        z: 999

        Text
        {
            id: backTxt
            anchors.centerIn: parent
            text: "← 返回"
            color: playRoot.g.mono ? Theme.acc : Theme.tx2
            font.pixelSize: 13
            font.family: playRoot.g.mono ? Theme.mono : Theme.family
        }

        MouseArea
        {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onEntered: backBtn.hovered = true
            onExited: backBtn.hovered = false
            onClicked:
            {
                playRoot.stack.pop()
                player.stopPreview()
            }
        }
    }

    // p10 右侧控制柱的底
    Rectangle
    {
        visible: playRoot.g.panelR
        x: st.x + st.width - playRoot.g.rw
        y: 0
        width: playRoot.g.rw
        height: playRoot.parent.height
        color: Theme.pan
        border.color: Theme.brd
        border.width: 1
    }

    // ===== 舞台 =====
    Item
    {
        id: st
        x: 36
        y: 78
        width: playRoot.width - 72
        height: playRoot.height - y - 116
        clip: true
    }

    // p7 玻璃卡 / p8 分隔线
    Rectangle
    {
        visible: playRoot.g.card
        x: st.x + playRoot.g.cardX
        y: st.y + playRoot.g.cardY
        width: playRoot.g.cardW
        height: playRoot.g.cardH
        radius: Theme.r3
        color: Theme.pan
        border.color: Theme.brd
        border.width: 1
    }

    Repeater
    {
        model: playRoot.g.dividers ? 2 : 0
        delegate: Rectangle
        {
            x: st.x + st.width * (index + 1) / 3
            y: st.y + 20
            width: 1
            height: st.height - 40
            color: Theme.brd
        }
    }

    // ===== 圆盘 + 环形频谱 =====
    Item
    {
        x: st.x + playRoot.g.discX
        y: st.y + playRoot.g.discY
        width: playRoot.g.discS
        height: playRoot.g.discS

        Disc10
        {
            anchors.centerIn: parent
            size: playRoot.g.discS
            coverSource: Theme.url(player.currentMusicCover)
            bars: playRoot.bars
            spin: player.playing
            ringColor: Theme.spec
            seed: playRoot.v
        }
    }

    // ===== 信息列：歌名 / 歌手 / 控制 / 进度 / 音量 =====
    Column
    {
        x: st.x + playRoot.g.infoX
        y: st.y + playRoot.g.infoY
        width: playRoot.g.infoW
        spacing: 10

        Text
        {
            width: parent.width
            text: player.currentMusicName
            color: Theme.tx
            font.pixelSize: playRoot.g.titlePx
            font.bold: true
            font.family: playRoot.g.mono ? Theme.mono : playRoot.g.titleFam
            horizontalAlignment: hAlign()
            elide: Text.ElideRight

            function hAlign()
            {
                if (playRoot.g.align === "center") return Text.AlignHCenter
                if (playRoot.g.align === "right") return Text.AlignRight
                return Text.AlignLeft
            }
        }

        Text
        {
            width: parent.width
            text: player.currentMusicSinger
            color: Theme.tx3
            font.pixelSize: Math.max(12, playRoot.g.titlePx * 0.5)
            font.family: playRoot.g.mono ? Theme.mono : Theme.family
            horizontalAlignment: playRoot.g.align === "center" ? Text.AlignHCenter
                                 : (playRoot.g.align === "right" ? Text.AlignRight : Text.AlignLeft)
            elide: Text.ElideRight
        }

        Item
        {
            width: parent.width
            height: playRoot.g.bigPx + 6

            Transport10
            {
                id: pgTrans
                btnPx: playRoot.g.ctlPx
                bigPx: playRoot.g.bigPx
                shape: 999
                fam: playRoot.g.mono ? Theme.mono : Theme.family
                anchors.verticalCenter: parent.verticalCenter
                x: playRoot.g.align === "center" ? (parent.width - width) / 2
                   : (playRoot.g.align === "right" ? parent.width - width : 0)
            }
        }

        Progress10
        {
            width: parent.width
            barW: parent.width
            fam: playRoot.g.mono ? Theme.mono : Theme.family
            timePx: playRoot.g.mono ? 11 : 11.5
            showTime: !playRoot.g.lrcSingle
        }

        // 音量
        Row
        {
            visible: playRoot.g.volRow
            spacing: 10
            x: playRoot.g.align === "center" ? (parent.width - width) / 2 : 0

            Text
            {
                anchors.verticalCenter: parent.verticalCenter
                text: "音量"
                color: Theme.tx3
                font.pixelSize: 11
                font.family: playRoot.g.mono ? Theme.mono : Theme.family
            }

            Slide10
            {
                width: Math.max(90, playRoot.g.infoW * 0.5)
                anchors.verticalCenter: parent.verticalCenter
                from: 0
                to: 1
                value: player.volume
                onMoved: player.volume = value
            }
        }
    }

    // ===== 歌词偏移竖条 =====
    Slide10
    {
        x: st.x + playRoot.g.offX
        y: st.y + playRoot.g.offY
        height: playRoot.g.offH
        width: 26
        orientation: Qt.Vertical
        from: -5000
        to: 5000
        stepSize: 100
        value: player.lyricOffset
        thickness: 3
        onMoved: player.lyricOffset = value
    }

    // ===== 歌词 =====
    Item
    {
        x: st.x + playRoot.g.lrcX
        y: st.y + playRoot.g.lrcY
        width: playRoot.g.lrcW
        height: playRoot.g.lrcH
        clip: true

        // 滚动式
        ListView
        {
            id: lrcView
            anchors.fill: parent
            visible: !playRoot.g.lrcSingle
            model: player.musicLrc
            spacing: 4
            highlightMoveDuration: 220
            boundsBehavior: Flickable.StopAtBounds

            delegate: Text
            {
                width: lrcView.width
                text: modelData.text
                horizontalAlignment: playRoot.g.align === "center" || playRoot.v === 7 || playRoot.v === 8
                                     ? Text.AlignHCenter : Text.AlignLeft
                color: index === playRoot.currentLrcIndex ? playRoot.lx() : Theme.tx2
                opacity: index === playRoot.currentLrcIndex ? 1 : 0.42
                font.pixelSize: playRoot.g.lrcPx * (index === playRoot.currentLrcIndex ? 1.12 : 1)
                font.bold: index === playRoot.currentLrcIndex
                font.family: playRoot.g.mono ? Theme.mono : Theme.family
                elide: Text.ElideRight

                Behavior on font.pixelSize { NumberAnimation { duration: 150 } }
                Behavior on opacity { NumberAnimation { duration: 150 } }
            }
        }

        // 单行式（p3 / p9）
        Text
        {
            visible: playRoot.g.lrcSingle
            anchors.centerIn: parent
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
            color: playRoot.lx()
            opacity: playRoot.currentLrcIndex >= 0 ? 1 : 0.4
            font.pixelSize: playRoot.g.lrcPx
            font.bold: true
            font.family: playRoot.g.mono ? Theme.mono : Theme.family
            text: playRoot.lrcLine()
        }
    }

    // player.lrcColor 是字符串，取不到就用主题的歌词色
    function lx()
    {
        return player.lrcColor
    }

    function lrcLine()
    {
        var mod = player.musicLrc
        if (!mod || mod.length === 0 || playRoot.currentLrcIndex < 0)
            return ""
        return mod[playRoot.currentLrcIndex].text
    }

    // ===== 歌词行号（照 Play.qml:24-56）=====
    Connections
    {
        target: player

        function onPositionChanged()
        {
            let pos = player.position + player.lyricOffset
            let mod = player.musicLrc
            if (!mod || mod.length === 0)
                return
            let idx = -1
            for (let i = 0; i < mod.length; i++)
            {
                if (i === mod.length - 1)
                {
                    idx = i
                    break
                }
                if (pos >= mod[i].time && pos < mod[i + 1].time)
                {
                    idx = i
                    break
                }
            }
            playRoot.currentLrcIndex = idx
        }
    }

    onCurrentLrcIndexChanged:
    {
        if (!playRoot.g.lrcSingle && currentLrcIndex >= 0)
            lrcView.positionViewAtIndex(currentLrcIndex, ListView.Center)
    }

    Connections
    {
        target: audioAnalyzer

        function onSpectrumChanged(data)
        {
            playRoot.bars = data
        }
    }
}
