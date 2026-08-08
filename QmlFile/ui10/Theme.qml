pragma Singleton
import QtQuick

// 10 套皮肤的唯一数据源：改这里就能改整套 UI
QtObject
{
    id: themeRoot

    // 当前第几套（1 - 10）
    property int scheme: 10

    readonly property int n: scheme < 1 ? 1 : (scheme > schemes.length ? schemes.length : scheme)
    readonly property var cur: schemes[n - 1]

    readonly property string name: cur.name
    readonly property color bg: cur.bg
    readonly property color tx: cur.tx
    readonly property color tx2: cur.tx2
    readonly property color tx3: cur.tx3
    readonly property color pan: cur.pan
    readonly property color pan2: cur.pan2
    readonly property color brd: cur.brd
    readonly property color hov: cur.hov
    readonly property color sel: cur.sel
    readonly property color rail: cur.rail
    readonly property color acc: cur.acc
    readonly property color acctx: cur.acctx
    readonly property color lyr: cur.lyr
    readonly property color spec: cur.spec
    readonly property color veil: cur.veil
    readonly property real veilA: cur.veilA
    readonly property color dockSol: cur.docksol
    readonly property int r1: cur.r1
    readonly property int r2: cur.r2
    readonly property int r3: cur.r3
    readonly property int blurPx: cur.blur
    readonly property int panBlur: cur.panblur
    readonly property string family: cur.family
    readonly property string serif: cur.serif
    readonly property string mono: cur.mono

    // 布局变体号：h=首页 p=播放页 d=双耳页 c=底栏
    readonly property int h: cur.h
    readonly property int p: cur.p
    readonly property int d: cur.d
    readonly property int c: cur.c

    // 尺寸旋钮（比例基于窗口短边，缩放窗口不塌）
    readonly property real panelWFrac: cur.panelWFrac
    readonly property real rowHFrac: cur.rowHFrac
    readonly property real rowCvFrac: cur.rowCvFrac
    readonly property real titlePx: cur.titlePx
    readonly property real partFrac: cur.partFrac
    readonly property real lyricPx: cur.lyricPx
    readonly property real dockH: cur.dockH
    readonly property bool light: cur.light

    // QUrl 有时带 scheme 有时不带（PlayMusic.cpp:348 存裸路径，:295-304 存完整 URL）
    function url(u)
    {
        var s = (u === undefined || u === null) ? "" : u.toString()
        if (s.length === 0)
            return ""
        if (s.indexOf("file:") === 0 || s.indexOf("qrc:") === 0 || s.indexOf("http") === 0)
            return s
        return "file:///" + s
    }

    function ms(v)
    {
        var t = Math.max(0, Math.round(v / 1000))
        var m = Math.floor(t / 60)
        var s = t % 60
        return (m < 10 ? "0" + m : "" + m) + ":" + (s < 10 ? "0" + s : "" + s)
    }

    readonly property var schemes: [
        {
            name: "1 极光深空",
            h: 1, p: 1, d: 1, c: 1,
            bg: "#05070F", tx: "#F2F5FF", tx2: "#99E2EAFF", tx3: "#57E2EAFF",
            pan: "#8C101626", pan2: "#0FFFFFFF", brd: "#17FFFFFF",
            hov: "#0FFFFFFF", sel: "#2122D3EE", rail: "#29FFFFFF",
            acc: "#22D3EE", acctx: "#04202A", lyr: "#7FE9FF", spec: "#22D3EE",
            veil: "#03050C", veilA: 0.55, docksol: "#8C101626",
            r1: 10, r2: 14, r3: 20, blur: 26, panblur: 18, light: false,
            family: "Microsoft YaHei", serif: "Georgia", mono: "Consolas",
            panelWFrac: 0.32, rowHFrac: 0.078, rowCvFrac: 0.052, titlePx: 28,
            partFrac: 0.56, lyricPx: 17, dockH: 74
        },
        {
            name: "2 现场红",
            h: 3, p: 2, d: 2, c: 2,
            bg: "#0C0A0A", tx: "#FFF5F2", tx2: "#99FFEBE6", tx3: "#52FFEBE6",
            pan: "#991E100E", pan2: "#0FFFFFFF", brd: "#14FFFFFF",
            hov: "#12FFFFFF", sel: "#29EC4141", rail: "#24FFFFFF",
            acc: "#EC4141", acctx: "#FFF5F2", lyr: "#FF8A7A", spec: "#F97316",
            veil: "#080404", veilA: 0.50, docksol: "#EB140A09",
            r1: 10, r2: 14, r3: 20, blur: 30, panblur: 18, light: false,
            family: "Microsoft YaHei", serif: "Georgia", mono: "Consolas",
            panelWFrac: 0.19, rowHFrac: 0.060, rowCvFrac: 0.040, titlePx: 28,
            partFrac: 0.62, lyricPx: 17, dockH: 90
        },
        {
            name: "3 奶油白",
            h: 4, p: 3, d: 3, c: 3,
            bg: "#E8E4DE", tx: "#2B2622", tx2: "#9E2B2622", tx3: "#612B2622",
            pan: "#9EFFFFFF", pan2: "#0A000000", brd: "#1A2B2622",
            hov: "#0D000000", sel: "#17FA243C", rail: "#292B2622",
            acc: "#FA243C", acctx: "#FFFFFF", lyr: "#D61F38", spec: "#FA243C",
            veil: "#F0ECE6", veilA: 0.50, docksol: "#9EFFFFFF",
            r1: 10, r2: 14, r3: 20, blur: 34, panblur: 20, light: true,
            family: "Microsoft YaHei", serif: "Georgia", mono: "Consolas",
            panelWFrac: 0.30, rowHFrac: 0.000, rowCvFrac: 0.000, titlePx: 28,
            partFrac: 0.46, lyricPx: 18, dockH: 66
        },
        {
            name: "4 索尼黑",
            h: 10, p: 5, d: 9, c: 4,
            bg: "#000000", tx: "#FFFFFF", tx2: "#99FFFFFF", tx3: "#4DFFFFFF",
            pan: "#C70E0E0E", pan2: "#0DFFFFFF", brd: "#24FFFFFF",
            hov: "#12FFFFFF", sel: "#21FFFFFF", rail: "#33FFFFFF",
            acc: "#FFFFFF", acctx: "#000000", lyr: "#FFFFFF", spec: "#9CA3AF",
            veil: "#000000", veilA: 0.42, docksol: "#C70E0E0E",
            r1: 4, r2: 6, r3: 10, blur: 34, panblur: 14, light: false,
            family: "Segoe UI", serif: "Georgia", mono: "Consolas",
            panelWFrac: 0.30, rowHFrac: 0.056, rowCvFrac: 0.042, titlePx: 28,
            partFrac: 0.26, lyricPx: 24, dockH: 90
        },
        {
            name: "5 蒸汽波",
            h: 2, p: 8, d: 7, c: 5,
            bg: "#0D0221", tx: "#F5ECFF", tx2: "#9EF0E1FF", tx3: "#57F0E1FF",
            pan: "#941C0A36", pan2: "#12FFFFFF", brd: "#1FFFFFFF",
            hov: "#1FFF71CE", sel: "#2EFF71CE", rail: "#29FFFFFF",
            acc: "#FF71CE", acctx: "#2B0A3D", lyr: "#01CDFE", spec: "#01CDFE",
            veil: "#0D0221", veilA: 0.42, docksol: "#941C0A36",
            r1: 14, r2: 20, r3: 24, blur: 24, panblur: 16, light: false,
            family: "Microsoft YaHei", serif: "Georgia", mono: "Consolas",
            panelWFrac: 0.27, rowHFrac: 0.078, rowCvFrac: 0.052, titlePx: 28,
            partFrac: 0.35, lyricPx: 15, dockH: 90
        },
        {
            name: "6 杂志纸",
            h: 7, p: 9, d: 6, c: 6,
            bg: "#F4EFE6", tx: "#16130E", tx2: "#A816130E", tx3: "#6616130E",
            pan: "#B3FFFCF6", pan2: "#0A16130E", brd: "#2916130E",
            hov: "#0D16130E", sel: "#14C8102E", rail: "#3316130E",
            acc: "#C8102E", acctx: "#FFF9F0", lyr: "#C8102E", spec: "#8C6D3F",
            veil: "#F4EFE6", veilA: 0.55, docksol: "#B3FFFCF6",
            r1: 6, r2: 10, r3: 14, blur: 36, panblur: 14, light: true,
            family: "Microsoft YaHei", serif: "Georgia", mono: "Consolas",
            panelWFrac: 0.30, rowHFrac: 0.084, rowCvFrac: 0.052, titlePx: 28,
            partFrac: 0.27, lyricPx: 19, dockH: 104
        },
        {
            name: "7 北欧海",
            h: 8, p: 4, d: 4, c: 7,
            bg: "#E9EFF3", tx: "#16303A", tx2: "#A316303A", tx3: "#6116303A",
            pan: "#99FFFFFF", pan2: "#0A16303A", brd: "#1A16303A",
            hov: "#0D16303A", sel: "#1A0F766E", rail: "#2916303A",
            acc: "#0F766E", acctx: "#F2FBF9", lyr: "#0D9488", spec: "#0EA5E9",
            veil: "#E9EFF3", veilA: 0.50, docksol: "#99FFFFFF",
            r1: 10, r2: 14, r3: 20, blur: 32, panblur: 20, light: true,
            family: "Microsoft YaHei", serif: "Georgia", mono: "Consolas",
            panelWFrac: 0.34, rowHFrac: 0.078, rowCvFrac: 0.052, titlePx: 28,
            partFrac: 0.52, lyricPx: 17, dockH: 60
        },
        {
            name: "8 影院金",
            h: 5, p: 6, d: 8, c: 8,
            bg: "#080604", tx: "#F5EBD7", tx2: "#99F5EBD7", tx3: "#52F5EBD7",
            pan: "#9E1A140C", pan2: "#0DFFFFFF", brd: "#2ED4AF37",
            hov: "#1AD4AF37", sel: "#29D4AF37", rail: "#29F5EBD7",
            acc: "#D4AF37", acctx: "#241A05", lyr: "#F0D68A", spec: "#D4AF37",
            veil: "#060402", veilA: 0.50, docksol: "#D9080604",
            r1: 8, r2: 12, r3: 16, blur: 26, panblur: 16, light: false,
            family: "Georgia", serif: "Georgia", mono: "Consolas",
            panelWFrac: 0.32, rowHFrac: 0.078, rowCvFrac: 0.052, titlePx: 56,
            partFrac: 0.25, lyricPx: 16, dockH: 104
        },
        {
            name: "9 终端绿",
            h: 9, p: 10, d: 10, c: 9,
            bg: "#020604", tx: "#D8FFD8", tx2: "#99C8FFC8", tx3: "#52C8FFC8",
            pan: "#D1041008", pan2: "#0D78FF78", brd: "#4084CC16",
            hov: "#1478FF78", sel: "#24B4FF39", rail: "#33B4FF78",
            acc: "#B4FF39", acctx: "#0A1A05", lyr: "#B4FF39", spec: "#84CC16",
            veil: "#020604", veilA: 0.46, docksol: "#F2030C06",
            r1: 6, r2: 8, r3: 10, blur: 24, panblur: 10, light: false,
            family: "Consolas", serif: "Georgia", mono: "Consolas",
            panelWFrac: 0.24, rowHFrac: 0.054, rowCvFrac: 0.038, titlePx: 20,
            partFrac: 0.33, lyricPx: 20, dockH: 64
        },
        {
            name: "10 白空",
            h: 6, p: 7, d: 5, c: 10,
            bg: "#EDF4FA", tx: "#12293E", tx2: "#9E12293E", tx3: "#5C12293E",
            pan: "#A8FFFFFF", pan2: "#0A12293E", brd: "#1712293E",
            hov: "#0D12293E", sel: "#1A0EA5E9", rail: "#2612293E",
            acc: "#0EA5E9", acctx: "#FFFFFF", lyr: "#0284C7", spec: "#0EA5E9",
            veil: "#EDF4FA", veilA: 0.46, docksol: "#A8FFFFFF",
            r1: 12, r2: 16, r3: 22, blur: 34, panblur: 22, light: true,
            family: "Microsoft YaHei", serif: "Georgia", mono: "Consolas",
            panelWFrac: 0.33, rowHFrac: 0.078, rowCvFrac: 0.052, titlePx: 64,
            partFrac: 0.37, lyricPx: 17, dockH: 70
        }
    ]
}
