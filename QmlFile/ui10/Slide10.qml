import QtQuick
import nusicvideoqml

// 主题化滑条。不用 Slider：Windows 原生样式会忽略 Slider 的 background/handle 自定义，
// 皮肤就白做了，所以这里直接用 Item + MouseArea 画
Item
{
    id: sl

    property real from: 0
    property real to: 1
    property real stepSize: 0
    property real value: 0
    property bool pressed: false
    property int orientation: Qt.Horizontal
    property color fillColor: Theme.acc
    property color railColor: Theme.rail
    property real thickness: 4
    property real knobPx: 12
    property bool showKnob: true

    signal moved()

    readonly property bool vert: orientation === Qt.Vertical
    readonly property real span: to - from
    readonly property real frac: span > 0 ? Math.max(0, Math.min(1, (value - from) / span)) : 0

    implicitHeight: vert ? 120 : Math.max(22, thickness + 12)
    implicitWidth: vert ? Math.max(22, thickness + 12) : 120

    function setValue(v)
    {
        var n = Math.max(from, Math.min(to, v))
        if (stepSize > 0)
            n = from + Math.round((n - from) / stepSize) * stepSize
        if (n !== value)
        {
            value = n
            sl.moved()
        }
    }

    // 轨道
    Rectangle
    {
        x: sl.vert ? (sl.width - sl.thickness) / 2 : 0
        y: sl.vert ? 0 : (sl.height - sl.thickness) / 2
        width: sl.vert ? sl.thickness : sl.width
        height: sl.vert ? sl.height : sl.thickness
        radius: sl.thickness / 2
        color: sl.railColor
    }

    // 已走过的部分
    Rectangle
    {
        x: sl.vert ? (sl.width - sl.thickness) / 2 : 0
        y: sl.vert ? sl.height - height : (sl.height - sl.thickness) / 2
        width: sl.vert ? sl.thickness : sl.width * sl.frac
        height: sl.vert ? sl.height * sl.frac : sl.thickness
        radius: sl.thickness / 2
        color: sl.fillColor
    }

    // 手柄
    Rectangle
    {
        visible: sl.showKnob
        width: sl.knobPx
        height: sl.knobPx
        radius: sl.knobPx / 2
        color: "white"
        border.color: sl.fillColor
        border.width: 1
        opacity: sl.enabled ? 1 : 0.4
        x: sl.vert ? (sl.width - width) / 2 : sl.width * sl.frac - width / 2
        y: sl.vert ? sl.height * (1 - sl.frac) - height / 2 : (sl.height - height) / 2
    }

    MouseArea
    {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor

        function fracAt(mx, my)
        {
            if (sl.vert)
                return 1 - Math.max(0, Math.min(1, my / Math.max(1, height)))
            return Math.max(0, Math.min(1, mx / Math.max(1, width)))
        }

        onPressed:
        {
            sl.pressed = true
            sl.setValue(sl.from + sl.span * fracAt(mouse.x, mouse.y))
        }
        onPositionChanged:
        {
            if (sl.pressed)
                sl.setValue(sl.from + sl.span * fracAt(mouse.x, mouse.y))
        }
        onReleased: sl.pressed = false
        onCanceled: sl.pressed = false
    }
}
