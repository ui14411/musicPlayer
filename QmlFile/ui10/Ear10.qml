import QtQuick
import QtQuick.Controls
import nusicvideoqml

// 双耳页的单侧播放器：耳标 + 带环形频谱的封面 + 曲名 + 独立进度 + 播放键
// vert = true 竖排一列，false 横向（封面在左，信息在右）
Item
{
    id: ear

    property string label: "左耳"
    property string name: ""
    property string cover: ""
    property real pos: 0
    property real dur: 1
    property bool playing: false

    property real coverPx: 150
    property bool vert: true
    property bool square: false
    property bool ring: true
    property bool tape: false
    property bool showProg: true
    property bool showBtn: true
    property int seed: 0
    property string fam: Theme.family
    property real labelPx: 13
    property real namePx: 15
    property real timePx: 11

    signal toggled()
    signal sought(real frac)

    readonly property var g: lay()

    function lay()
    {
        var W = ear.width, H = ear.height
        var o = {}
        if (ear.vert)
        {
            var col = Math.min(W - 8, ear.coverPx + 90)
            var cx = (W - col) / 2
            var used = ear.labelPx + 12 + ear.coverPx + (ear.tape ? 44 : 0)
                       + (ear.showProg ? 30 : 0) + (ear.showBtn ? 44 : 0) + (ear.namePx + 8)
            var y = Math.max(4, (H - used) / 2)
            o.lab = { x: cx, y: y, w: col, h: ear.labelPx + 6 }
            y += o.lab.h + 6
            o.disc = { x: cx + (col - ear.coverPx) / 2, y: y, w: ear.coverPx, h: ear.coverPx }
            y += ear.coverPx + (ear.tape ? 10 : 6)
            if (ear.tape) { o.tapeR = { x: cx + (col - 170) / 2, y: y, w: Math.min(170, col), h: 34 }; y += 44 }
            o.nm = { x: cx, y: y, w: col, h: ear.namePx + 6 }
            y += o.nm.h + 6
            if (ear.showProg) { o.prog = { x: cx, y: y, w: col, h: 24 }; y += 30 }
            if (ear.showBtn) { o.btn = { x: cx + (col - 42) / 2, y: y, w: 42, h: 42 } }
        }
        else
        {
            var cw = ear.coverPx
            var iw = W - cw - 16
            var ih = Math.min(H, 96)
            var top = (H - ih) / 2
            o.disc = { x: 0, y: (H - cw) / 2, w: cw, h: cw }
            o.lab = { x: cw + 16, y: top, w: iw, h: ear.labelPx + 4 }
            o.nm = { x: cw + 16, y: top + o.lab.h + 2, w: iw, h: ear.namePx + 6 }
            if (ear.showProg) o.prog = { x: cw + 16, y: top + ih - 24, w: iw, h: 24 }
            if (ear.showBtn) o.btn = { x: cw + 16 + iw - 42, y: o.lab.y - 46, w: 42, h: 42 }
        }
        return o
    }

    // 封面圆盘
    Item
    {
        x: ear.g.disc.x
        y: ear.g.disc.y
        width: ear.g.disc.w
        height: ear.g.disc.h

        Disc10
        {
            anchors.centerIn: parent
            size: Math.min(parent.width, parent.height)
            coverSource: Theme.url(ear.cover)
            square: ear.square
            showRing: ear.ring
            spin: ear.playing
            seed: ear.seed
            ringColor: Theme.spec
        }
    }

    // 耳标
    Text
    {
        x: ear.g.lab.x
        y: ear.g.lab.y
        width: ear.g.lab.w
        height: ear.g.lab.h
        verticalAlignment: Text.AlignVCenter
        text: ear.label
        color: Theme.tx3
        font.pixelSize: ear.labelPx
        font.bold: true
        font.letterSpacing: ear.vert ? 2 : 1
        font.family: ear.fam
        horizontalAlignment: ear.vert ? Text.AlignHCenter : Text.AlignLeft
    }

    // 曲名
    Text
    {
        x: ear.g.nm.x
        y: ear.g.nm.y
        width: ear.g.nm.w
        height: ear.g.nm.h
        verticalAlignment: Text.AlignVCenter
        text: ear.name
        color: Theme.tx
        font.pixelSize: ear.namePx
        font.bold: true
        font.family: ear.fam
        elide: Text.ElideRight
        horizontalAlignment: ear.vert ? Text.AlignHCenter : Text.AlignLeft
    }

    // 磁带窗（d7）
    Rectangle
    {
        visible: ear.tape
        x: ear.g.tapeR ? ear.g.tapeR.x : 0
        y: ear.g.tapeR ? ear.g.tapeR.y : 0
        width: ear.g.tapeR ? ear.g.tapeR.w : 0
        height: ear.g.tapeR ? ear.g.tapeR.h : 0
        radius: 8
        border.color: Theme.brd
        border.width: 1
        color: Theme.pan2

        Row
        {
            anchors.centerIn: parent
            spacing: 26

            Repeater
            {
                model: 2
                delegate: Rectangle
                {
                    width: 16
                    height: 16
                    radius: 8
                    color: "transparent"
                    border.color: Theme.tx3
                    border.width: 2.5

                    NumberAnimation on rotation
                    {
                        from: 0
                        to: 360
                        duration: 6000
                        loops: Animation.Infinite
                        running: ear.playing
                    }
                }
            }
        }
    }

    // 独立进度：当前时间 — 条 — 总时长
    Item
    {
        visible: ear.showProg
        x: ear.g.prog ? ear.g.prog.x : 0
        y: ear.g.prog ? ear.g.prog.y : 0
        width: ear.g.prog ? ear.g.prog.w : 0
        height: ear.g.prog ? ear.g.prog.h : 0

        Row
        {
            anchors.fill: parent
            spacing: 8

            Text
            {
                anchors.verticalCenter: parent.verticalCenter
                width: 34
                horizontalAlignment: Text.AlignRight
                text: Theme.ms(ear.pos)
                color: Theme.tx3
                font.pixelSize: ear.timePx
                font.family: ear.fam
            }

            Slide10
            {
                id: earSl
                width: parent.width - 84
                anchors.verticalCenter: parent.verticalCenter
                from: 0
                to: 1
                thickness: 3

                onMoved: ear.sought(value)

                // 拖动时不跟内部值打架，松手后再同步
                Connections
                {
                    target: ear

                    function onPosChanged()
                    {
                        if (!earSl.pressed && ear.dur > 0)
                            earSl.value = ear.pos / ear.dur
                    }

                    function onDurChanged()
                    {
                        earSl.value = 0
                    }
                }
            }

            Text
            {
                anchors.verticalCenter: parent.verticalCenter
                width: 34
                text: Theme.ms(ear.dur)
                color: Theme.tx3
                font.pixelSize: ear.timePx
                font.family: ear.fam
            }
        }
    }

    // 播放 / 暂停
    Rectangle
    {
        visible: ear.showBtn
        x: ear.g.btn ? ear.g.btn.x : 0
        y: ear.g.btn ? ear.g.btn.y : 0
        width: 42
        height: 42
        radius: 21
        color: Theme.acc
        scale: ma2.containsMouse ? 1.06 : 1
        Behavior on scale { NumberAnimation { duration: 120 } }

        Text
        {
            anchors.centerIn: parent
            text: ear.playing ? "⏸" : "▶"
            color: Theme.acctx
            font.pixelSize: 16
        }

        MouseArea
        {
            id: ma2
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: ear.toggled()
        }
    }
}
