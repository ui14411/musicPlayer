import QtQuick
import QtQuick.Controls
import Qt5Compat.GraphicalEffects
import nusicvideoqml

// 双耳页：10 套布局（d1 - d10）
// 背景固定 = 首页选好的静态图 player.bgPath 模糊 + 极慢推近（和 PlayDouble.qml 一致）
Item
{
    id: dblRoot

    property var stack
    objectName: "doublePage"

    readonly property int v: Theme.d
    readonly property var g: lay()

    property int leftLrcIndex: -1
    property int rightLrcIndex: -1

    Component.onCompleted:
    {
        stack = StackView.view
    }

    // ===== 几何表 =====
    function lay()
    {
        var W = dst.width
        var H = dst.height
        var o = {
            earL: { x: 0, y: 0, w: 0, h: 0 },
            earR: { x: 0, y: 0, w: 0, h: 0 },
            lrcL: { x: 0, y: 0, w: 0, h: 0 },
            lrcR: { x: 0, y: 0, w: 0, h: 0 },
            volL: { x: 0, y: 0, h: 0, show: true },
            volR: { x: 0, y: 0, h: 0, show: true },
            hub: false, hubX: 0, stamp: false, centerLine: false,
            vert: true, square: false, tape: false, hideVol: false,
            mono: false, serif: false,
            coverPx: Math.min(150, H * 0.26, W * 0.13),
            lrcPx: 14.5, namePx: 15, labelPx: 13, timePx: 11,
            gap: 12, deco: [], fam: Theme.family
        }

        // 六段一排：耳 | 音量 | 词 | 词 | 音量 | 耳
        function flow6(gap, coverPx, cards)
        {
            o.coverPx = coverPx
            o.gap = gap
            var colW = coverPx + 86
            var volW = o.hideVol ? 0 : 30
            var lrcW = Math.max(120, (W - 2 * colW - 2 * volW - 5 * gap) / 2)
            var x = 0

            o.earL.x = x; o.earL.y = 0; o.earL.w = colW; o.earL.h = H
            if (cards) o.deco.push({ x: x, y: 0, w: colW, h: H })
            x += colW + gap

            if (!o.hideVol)
            {
                o.volL.x = x + 6; o.volL.y = H * 0.2; o.volL.h = H * 0.6; o.volL.show = true
                x += volW + gap
            }
            o.lrcL.x = x; o.lrcL.y = 0; o.lrcL.w = lrcW; o.lrcL.h = H
            if (cards) o.deco.push({ x: x, y: 0, w: lrcW, h: H })
            x += lrcW + gap

            o.lrcR.x = x; o.lrcR.y = 0; o.lrcR.w = lrcW; o.lrcR.h = H
            if (cards) o.deco.push({ x: x, y: 0, w: lrcW, h: H })
            x += lrcW + gap

            if (!o.hideVol)
            {
                o.volR.x = x + 6; o.volR.y = H * 0.2; o.volR.h = H * 0.6; o.volR.show = true
                x += volW + gap
            }
            o.earR.x = x; o.earR.y = 0; o.earR.w = colW; o.earR.h = H
            if (cards) o.deco.push({ x: x, y: 0, w: colW, h: H })
        }

        switch (v)
        {
        case 1: // 六段一排（他现在的排布，换成主题皮）
            flow6(12, o.coverPx, false)
            break

        case 2: // 对称 + 中央枢纽
            o.hub = true
            flow6(14, o.coverPx, true)
            break

        case 3: // 每段独立玻璃卡
            flow6(14, o.coverPx, true)
            break

        case 4: // 上下分区：上双列 / 下双词
        {
            o.vert = false
            o.coverPx = Math.min(110, H * 0.2, W * 0.1)
            var topH = Math.max(120, H * 0.34)
            var half = (W - 24) / 2
            o.earL.x = 0; o.earL.y = 0; o.earL.w = half - 40; o.earL.h = topH
            o.earR.x = W - (half - 40); o.earR.y = 0; o.earR.w = half - 40; o.earR.h = topH
            o.volL.show = true; o.volL.x = half - 34; o.volL.y = 12; o.volL.h = topH - 24
            o.volR.show = true; o.volR.x = W - half + 4; o.volR.y = 12; o.volR.h = topH - 24
            o.lrcL.x = 0; o.lrcL.y = topH + 16; o.lrcL.w = half - 8; o.lrcL.h = H - topH - 16
            o.lrcR.x = half + 8; o.lrcR.y = topH + 16; o.lrcR.w = half - 8; o.lrcR.h = H - topH - 16
            o.deco.push({ x: 0, y: 0, w: half - 20, h: topH })
            o.deco.push({ x: W - half + 20, y: 0, w: half - 20, h: topH })
            o.deco.push({ x: 0, y: topH + 16, w: half - 8, h: H - topH - 16 })
            o.deco.push({ x: half + 8, y: topH + 16, w: half - 8, h: H - topH - 16 })
            break
        }

        case 5: // 上封面（横排带环） / 下宽词
        {
            o.vert = false
            o.hideVol = true
            o.stamp = true
            o.coverPx = Math.min(96, H * 0.16, W * 0.08)
            var th = Math.max(140, H * 0.36)
            var hw = (W - 16) / 2
            o.earL.x = 0; o.earL.y = 0; o.earL.w = hw - 8; o.earL.h = th
            o.earR.x = hw + 8; o.earR.y = 0; o.earR.w = hw - 8; o.earR.h = th
            o.lrcL.x = 0; o.lrcL.y = th + 16; o.lrcL.w = hw - 8; o.lrcL.h = H - th - 16
            o.lrcR.x = hw + 8; o.lrcR.y = th + 16; o.lrcR.w = hw - 8; o.lrcR.h = H - th - 16
            o.deco.push({ x: 0, y: 0, w: hw - 8, h: th })
            o.deco.push({ x: hw + 8, y: 0, w: hw - 8, h: th })
            o.deco.push({ x: 0, y: th + 16, w: hw - 8, h: H - th - 16 })
            o.deco.push({ x: hw + 8, y: th + 16, w: hw - 8, h: H - th - 16 })
            break
        }

        case 6: // 报纸双栏：衬线 + 细线，音量藏起来
            o.hideVol = true
            o.serif = true
            o.square = true
            o.centerLine = true
            o.fam = Theme.serif
            o.lrcPx = 15.5
            o.coverPx = Math.min(130, H * 0.22, W * 0.11)
            flow6(0, o.coverPx, false)
            break

        case 7: // 磁带卡：方封面 + 带窗
            o.square = true
            o.tape = true
            o.coverPx = Math.min(150, H * 0.24, W * 0.12)
            flow6(14, o.coverPx, true)
            break

        case 8: // 镜像分屏，中央留空
        {
            o.hideVol = true
            o.coverPx = Math.min(120, H * 0.2, W * 0.1)
            var gw = (W - 180) / 2
            o.earL.x = 0; o.earL.y = 0; o.earL.w = gw - 12; o.earL.h = H
            // 左右各一张卡：卡里塞 耳 + 词，中间空 180 给背景
            o.earL.x = 0; o.earL.y = 0; o.earL.w = 150; o.earL.h = H
            o.lrcL.x = 162; o.lrcL.y = 0; o.lrcL.w = gw - 162; o.lrcL.h = H
            o.earR.x = W - 150; o.earR.y = 0; o.earR.w = 150; o.earR.h = H
            o.lrcR.x = W - gw + 12; o.lrcR.y = 0; o.lrcR.w = gw - 162; o.lrcR.h = H
            o.deco.push({ x: 0, y: 0, w: gw, h: H })
            o.deco.push({ x: W - gw, y: 0, w: gw, h: H })
            break
        }

        case 9: // 紧凑 mono 密度
            o.mono = true
            o.fam = Theme.mono
            o.coverPx = Math.min(100, H * 0.18, W * 0.09)
            o.lrcPx = 12.5
            o.namePx = 13
            o.labelPx = 11
            o.timePx = 10
            flow6(10, o.coverPx, true)
            break

        default: // 10 双终端窗口（带标题栏）
        {
            o.mono = true
            o.fam = Theme.mono
            o.hideVol = true
            o.coverPx = Math.min(110, H * 0.18, W * 0.09)
            o.lrcPx = 13
            var tw = (W - 18) / 2
            var barH = 38
            o.deco.push({ x: 0, y: 0, w: tw - 9, h: H, term: true, label: "LEFT.log" })
            o.deco.push({ x: tw + 9, y: 0, w: tw - 9, h: H, term: true, label: "RIGHT.log" })
            var inner = tw - 9 - 24
            o.earL.x = 12; o.earL.y = barH + 10; o.earL.w = inner * 0.44; o.earL.h = H - barH - 22
            o.lrcL.x = 12 + inner * 0.46; o.lrcL.y = barH + 10
            o.lrcL.w = inner * 0.52; o.lrcL.h = H - barH - 22
            o.earR.x = tw + 21; o.earR.y = barH + 10; o.earR.w = inner * 0.44; o.earR.h = H - barH - 22
            o.lrcR.x = tw + 21 + inner * 0.46; o.lrcR.y = barH + 10
            o.lrcR.w = inner * 0.52; o.lrcR.h = H - barH - 22
            break
        }
        }

        // 中央枢纽（d2）：一条竖线 + 两个点
        if (o.hub)
            o.hubX = W / 2

        return o
    }

    // ===== 背景：静态图 + 模糊 + 慢推 =====
    Item
    {
        id: bgLayer
        anchors.fill: parent
        clip: true

        Image
        {
            id: bgImage
            anchors.centerIn: parent
            width: parent.width * 1.1
            height: parent.height * 1.1
            source: Theme.url(player.bgPath)
            fillMode: Image.PreserveAspectCrop
            cache: false

            layer.enabled: true
            layer.effect: FastBlur
            {
                radius: 32
            }

            SequentialAnimation
            {
                loops: Animation.Infinite
                running: bgImage.status === Image.Ready

                NumberAnimation
                {
                    target: bgImage
                    property: "scale"
                    from: 1.0
                    to: 1.1
                    duration: 42000
                    easing.type: Easing.InOutSine
                }

                NumberAnimation
                {
                    target: bgImage
                    property: "scale"
                    from: 1.1
                    to: 1.0
                    duration: 42000
                    easing.type: Easing.InOutSine
                }
            }
        }
    }

    Rectangle
    {
        anchors.fill: parent
        color: Theme.veil
        opacity: 0.45
    }

    // ===== 返回 =====
    Rectangle
    {
        id: dblBack
        property bool hovered: false
        anchors.left: parent.left
        anchors.leftMargin: 30
        anchors.top: parent.top
        anchors.topMargin: 22
        width: 74
        height: 34
        radius: height / 2
        color: hovered ? Theme.pan2 : "transparent"
        z: 999

        Text
        {
            anchors.centerIn: parent
            text: "← 返回"
            color: dblBack.hovered ? Theme.tx : Theme.tx2
            font.pixelSize: 13
            font.family: dblRoot.g.mono ? Theme.mono : Theme.family
        }

        MouseArea
        {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onEntered: dblBack.hovered = true
            onExited: dblBack.hovered = false
            onClicked: dblRoot.stack.pop()
        }
    }

    // ===== 舞台 =====
    Item
    {
        id: dst
        x: 32
        y: 80
        width: dblRoot.width - 64
        height: dblRoot.height - y - 112
        clip: true
    }

    // 卡 / 终端窗
    Repeater
    {
        model: dblRoot.g.deco

        delegate: Rectangle
        {
            x: dst.x + modelData.x
            y: dst.y + modelData.y
            width: modelData.w
            height: modelData.h
            radius: modelData.term ? 12 : Theme.r2
            color: Theme.pan
            border.color: Theme.brd
            border.width: 1
            visible: modelData.w > 0

            // 终端标题栏
            Rectangle
            {
                visible: modelData.term === true
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                height: 38
                color: Theme.pan2
                border.color: Theme.brd
                border.width: 1

                Row
                {
                    anchors.left: parent.left
                    anchors.leftMargin: 14
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 7

                    Repeater
                    {
                        model: 3
                        delegate: Rectangle
                        {
                            width: 11
                            height: 11
                            radius: 6
                            color: ["#FF5F57", "#FEBC2E", "#28C840"][index]
                        }
                    }

                    Text
                    {
                        anchors.verticalCenter: parent.verticalCenter
                        leftPadding: 8
                        text: modelData.label
                        color: Theme.tx3
                        font.pixelSize: 12
                        font.family: Theme.mono
                    }
                }
            }
        }
    }

    // 中央枢纽（d2）
    Item
    {
        visible: dblRoot.g.hub
        x: dst.x + dblRoot.g.hubX - 13
        y: dst.y
        width: 26
        height: dst.height

        Rectangle
        {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 1
            color: Theme.brd
        }

        Repeater
        {
            model: 2
            delegate: Rectangle
            {
                width: 10
                height: 10
                radius: 5
                color: Theme.acc
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenterOffset: index === 0 ? -60 : 60
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    // d6 的中缝细线
    Rectangle
    {
        visible: dblRoot.g.centerLine
        x: dst.x + dst.width / 2
        y: dst.y + 20
        width: 1
        height: dst.height - 40
        color: Theme.brd
    }

    // ===== 左右耳播放器 =====
    Ear10
    {
        label: "左耳"
        x: dst.x + dblRoot.g.earL.x
        y: dst.y + dblRoot.g.earL.y
        width: dblRoot.g.earL.w
        height: dblRoot.g.earL.h
        vert: dblRoot.g.vert
        square: dblRoot.g.square
        tape: dblRoot.g.tape
        coverPx: dblRoot.g.coverPx
        fam: dblRoot.g.fam
        namePx: dblRoot.g.namePx
        labelPx: dblRoot.g.labelPx
        timePx: dblRoot.g.timePx
        name: Dplay.leftMusicName
        cover: Dplay.leftCover
        pos: Dplay.leftPosition
        dur: Dplay.leftMusicDuration
        playing: Dplay.leftPlaying
        seed: 1
        onToggled: Dplay.leftPlaying ? Dplay.stopLeftMusic() : Dplay.playLeft()
        onSought: function(frac) { Dplay.setLeftPos(frac * Dplay.leftMusicDuration) }
    }

    Ear10
    {
        label: "右耳"
        x: dst.x + dblRoot.g.earR.x
        y: dst.y + dblRoot.g.earR.y
        width: dblRoot.g.earR.w
        height: dblRoot.g.earR.h
        vert: dblRoot.g.vert
        square: dblRoot.g.square
        tape: dblRoot.g.tape
        coverPx: dblRoot.g.coverPx
        fam: dblRoot.g.fam
        namePx: dblRoot.g.namePx
        labelPx: dblRoot.g.labelPx
        timePx: dblRoot.g.timePx
        name: Dplay.rightMusicName
        cover: Dplay.rightCover
        pos: Dplay.rightPosition
        dur: Dplay.rightMusicDuration
        playing: Dplay.rightPlaying
        seed: 2
        onToggled: Dplay.rightPlaying ? Dplay.stopRightMusic() : Dplay.playRight()
        onSought: function(frac) { Dplay.setRightPos(frac * Dplay.rightMusicDuration) }
    }

    // ===== 音量（竖条）=====
    Column
    {
        visible: dblRoot.g.volL.show
        x: dst.x + dblRoot.g.volL.x
        y: dst.y + dblRoot.g.volL.y
        spacing: 8

        Text
        {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "L"
            color: Theme.tx3
            font.pixelSize: 10
            font.family: dblRoot.g.fam
        }

        Slide10
        {
            height: dblRoot.g.volL.h
            width: 26
            orientation: Qt.Vertical
            thickness: 3
            from: 0
            to: 1
            value: Dplay.leftVolume
            onMoved: Dplay.setLeftVolume(value)
        }
    }

    Column
    {
        visible: dblRoot.g.volR.show
        x: dst.x + dblRoot.g.volR.x
        y: dst.y + dblRoot.g.volR.y
        spacing: 8

        Text
        {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "R"
            color: Theme.tx3
            font.pixelSize: 10
            font.family: dblRoot.g.fam
        }

        Slide10
        {
            height: dblRoot.g.volR.h
            width: 26
            orientation: Qt.Vertical
            thickness: 3
            from: 0
            to: 1
            value: Dplay.rightVolume
            onMoved: Dplay.setRightVolume(value)
        }
    }

    // ===== 两条歌词 =====
    LrcCol10
    {
        x: dst.x + dblRoot.g.lrcL.x
        y: dst.y + dblRoot.g.lrcL.y
        width: dblRoot.g.lrcL.w
        height: dblRoot.g.lrcL.h
        model2: Dplay.leftLrc
        activeIdx: dblRoot.leftLrcIndex
        px: dblRoot.g.lrcPx
        fam: dblRoot.g.fam
        center: !dblRoot.g.serif
        hiColor: Theme.lyr
    }

    LrcCol10
    {
        x: dst.x + dblRoot.g.lrcR.x
        y: dst.y + dblRoot.g.lrcR.y
        width: dblRoot.g.lrcR.w
        height: dblRoot.g.lrcR.h
        model2: Dplay.rightLrc
        activeIdx: dblRoot.rightLrcIndex
        px: dblRoot.g.lrcPx
        fam: dblRoot.g.fam
        center: !dblRoot.g.serif
        hiColor: Theme.acc
    }

    // d5 底部说明水印
    Text
    {
        visible: dblRoot.g.stamp
        x: 40
        y: dst.y + dst.height - 34
        width: dst.width - 80
        horizontalAlignment: Text.AlignHCenter
        text: "双耳 = 一个列表条目（左右耳两首），每侧进度独立"
        color: Theme.tx3
        font.pixelSize: 11
        font.family: Theme.family
    }

    // ===== 行号计算（照 PlayDouble.qml:501-531）=====
    Connections
    {
        target: Dplay

        function onLeftPosChanged()
        {
            let pos = Dplay.leftPosition
            let list = Dplay.leftLrc
            if (!list || list.length === 0)
                return
            for (let i = 0; i < list.length; i++)
            {
                if (i === list.length - 1 || (pos >= list[i].time && pos < list[i + 1].time))
                {
                    dblRoot.leftLrcIndex = i
                    break
                }
            }
        }

        function onRightPosChanged()
        {
            let pos = Dplay.rightPosition
            let list = Dplay.rightLrc
            if (!list || list.length === 0)
                return
            for (let i = 0; i < list.length; i++)
            {
                if (i === list.length - 1 || (pos >= list[i].time && pos < list[i + 1].time))
                {
                    dblRoot.rightLrcIndex = i
                    break
                }
            }
        }
    }
}
