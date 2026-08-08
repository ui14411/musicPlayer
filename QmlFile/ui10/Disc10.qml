import QtQuick
import Qt5Compat.GraphicalEffects
import nusicvideoqml

// 封面圆盘 + 环形频谱（Play.qml / PlayDouble.qml 共用）
Item
{
    id: discRoot

    property real size: 300
    property url coverSource: ""
    property bool square: false
    property bool spin: false
    property int spinMs: 24000

    // 真频谱：audioAnalyzer 的 bars；没有就用 energy 画一圈呼吸（双耳每侧的真频谱要等 C++ 第二路）
    property var bars: []
    property real energy: 0.16
    property int seed: 0
    property bool showRing: true
    property color ringColor: Theme.spec
    property bool coverShadow: true

    width: size
    height: size

    readonly property real coverPx: size * 0.78
    readonly property real radius: size / 2
    readonly property real maxBar: size * 0.12

    Rectangle
    {
        id: discBg
        anchors.centerIn: parent
        width: coverPx
        height: coverPx
        radius: discRoot.square ? Theme.r1 : width / 2
        color: Theme.pan
        border.color: Theme.brd
        border.width: discRoot.square ? 1 : 0
    }

    Image
    {
        id: discCover
        anchors.centerIn: parent
        width: coverPx
        height: coverPx
        source: discRoot.coverSource
        fillMode: Image.PreserveAspectCrop
        smooth: true
        rotation: rotAnim.angle
        layer.enabled: true
        layer.effect: OpacityMask
        {
            maskSource: Rectangle
            {
                width: discCover.width
                height: discCover.height
                radius: discRoot.square ? Theme.r1 : width / 2
                color: "white"
            }
        }
    }

    PropertyAnimation
    {
        id: rotAnim
        target: discCover
        property: "rotation"
        from: 0
        to: 360
        duration: discRoot.spinMs
        loops: Animation.Infinite
        running: discRoot.spin
    }

    Canvas
    {
        id: discRing
        anchors.fill: parent
        visible: discRoot.showRing
        onPaint:
        {
            let g = getContext("2d")
            g.reset()
            g.clearRect(0, 0, width, height)

            let cx = width / 2
            let cy = height / 2
            let r0 = discRoot.coverPx / 2 + 6
            let count = 72
            let useBars = (discRoot.bars !== undefined && discRoot.bars !== null && discRoot.bars.length > 0)
            let lw = Math.max(1.6, discRoot.size * 0.008)

            g.strokeStyle = discRoot.ringColor
            g.lineWidth = lw
            g.lineCap = "round"

            for (let i = 0; i < count; ++i)
            {
                let v = 0
                if (useBars)
                {
                    v = Math.min(Number(discRoot.bars[i % discRoot.bars.length]) * 50, discRoot.maxBar)
                }
                else
                {
                    let t = Date.now() / 1000
                    let w1 = Math.sin(i * 0.37 + t * 1.6 + discRoot.seed * 2.4)
                    let w2 = Math.sin(i * 0.11 - t * 0.9 + discRoot.seed)
                    v = (0.18 + Math.abs(w1 * 0.7 + w2 * 0.3) * 0.5) * discRoot.maxBar * discRoot.energy
                }

                if (v < 0.6)
                    continue

                let a = (i / count) * Math.PI * 2 - Math.PI / 2
                g.beginPath()
                g.moveTo(cx + Math.cos(a) * r0, cy + Math.sin(a) * r0)
                g.lineTo(cx + Math.cos(a) * (r0 + v), cy + Math.sin(a) * (r0 + v))
                g.stroke()
            }
        }
    }

    Timer
    {
        interval: 60
        repeat: true
        running: discRoot.showRing && (discRoot.bars === undefined || discRoot.bars === null || discRoot.bars.length === 0)
        onTriggered: discRing.requestPaint()
    }

    DropShadow
    {
        anchors.fill: discBg
        source: discBg
        visible: discRoot.coverShadow
        horizontalOffset: 0
        verticalOffset: Math.max(4, discRoot.size * 0.02)
        radius: 18
        samples: 24
        color: "#73000000"
        cached: true
    }

    onBarsChanged: discRing.requestPaint()
    onEnergyChanged: discRing.requestPaint()
    onSizeChanged: discRing.requestPaint()
    onRingColorChanged: discRing.requestPaint()
    onShowRingChanged: discRing.requestPaint()
}
