import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtMultimedia
import Qt5Compat.GraphicalEffects
import nusicvideoqml

// 首页：10 套布局（h1 - h10），数据源和交互与 Home.qml 一致
Item
{
    id: homeRoot

    property var stack
    property bool imageORvideo: false
    property int panelMode: player.panelModel
    property int detIndex: -1

    readonly property int h: Theme.h
    readonly property var g: geo()
    readonly property bool gridMode: g.grid
    readonly property real cellW: 178
    readonly property real gridCardH: (cellW - 20) + 62
    readonly property real rowW: gridMode ? cellW - 12 : listArea.width - 6
    property string pendingPath: ""
    property int pendingKind: 0
    property bool previewOn: false

    function geo()
    {
        var W = homeRoot.width
        var H = homeRoot.height
        var o = {
            padX: 36, padT: 22, padB: 108, gap: 24, railW: 0, stats: false,
            panelW: Math.min(400, W * 0.36), panelBottom: false, grid: false,
            bare: false, hairline: false, idxSerif: false, squareCv: false,
            tabs: "strip", titlePx: Theme.titlePx, titleVertical: false,
            titleSerif: false, rowH: Theme.rowHFrac * H / 0.9, cvPx: Theme.rowCvFrac * H / 0.9,
            panelBordered: false
        }

        switch (h)
        {
        case 1:
            break
        case 2: // 左侧竖排 tab 轨
            o.railW = 76; o.tabs = "rail"; o.panelW = Math.min(340, W * 0.32)
            break
        case 3: // 全宽大表 + 窄侧板
            o.panelW = Math.min(240, W * 0.24); o.gap = 18
            o.rowH = Math.max(34, H * 0.055); o.cvPx = 28; o.tabs = "column"
            break
        case 4: // 封面卡网格 + 底部横板
            o.panelBottom = true; o.grid = true; o.tabs = "pills"
            o.panelW = W - o.padX * 2
            break
        case 5: // 左侧竖写大标题
            o.railW = 92; o.titleVertical = true
            break
        case 6: // 超大标题 + 无玻璃列表
            o.bare = true; o.hairline = true; o.panelW = Math.min(420, W * 0.38)
            break
        case 7: // 杂志双栏
            o.bare = true; o.hairline = true; o.idxSerif = true; o.squareCv = true
            o.titleSerif = true; o.gap = 32; o.rowH = Math.max(44, H * 0.084)
            o.panelW = Math.min(380, W * 0.34)
            break
        case 8: // tab 提到标题行下面
            o.tabs = "barrow"; o.panelW = Math.min(440, W * 0.4); o.gap = 16
            o.panelBordered = true
            break
        case 9: // 紧凑 + 实心抽屉
            o.padX = 28; o.panelW = Math.min(300, W * 0.28); o.gap = 16
            o.rowH = Math.max(32, H * 0.052); o.cvPx = 26; o.titlePx = 20
            o.panelBordered = true
            break
        default: // 10 三卡仪表盘
            o.stats = true; o.railW = 200; o.panelW = Math.min(380, W * 0.34)
            break
        }
        return o
    }

    Component.onCompleted:
    {
        stack = StackView.view
        player.setbgImage()
    }

    // ===== 预览：完全照他 Home.qml 的时序 =====
    Timer
    {
        id: hoverTimer
        repeat: false
        onTriggered: doPreview(homeRoot.pendingKind, homeRoot.pendingPath)
    }

    Timer
    {
        id: stopTimer
        repeat: false
        interval: 500
        onTriggered: stopPreview()
    }

    function stopPreview()
    {
        homeRoot.imageORvideo = false
        homeRoot.pendingPath = ""
        player.playPreviewMusic("")
    }

    function beginPreview(kind, path)
    {
        homeRoot.pendingKind = kind
        homeRoot.pendingPath = path
        stopTimer.stop()
        hoverTimer.interval = kind === 0 ? 800 : 500
        hoverTimer.restart()
    }

    function endPreview(kind)
    {
        hoverTimer.stop()
        if (kind === 0)
            stopTimer.restart()
        else
        {
            stopTimer.stop()
            stopPreview()
        }
    }

    function doPreview(kind, path)
    {
        if (kind === 2)
        {
            homeRoot.imageORvideo = true
            player.playPreviewImage(path)
        }
        else
        {
            homeRoot.imageORvideo = false
            player.playPreviewMusic(path)
        }
    }

    // ===== 背景 =====
    Image
    {
        id: homeBg
        anchors.fill: parent
        source: Theme.url(player.bgPath)
        fillMode: Image.PreserveAspectCrop

        layer.enabled: Theme.light
        layer.effect: FastBlur
        {
            radius: 12
        }
    }

    Rectangle
    {
        anchors.fill: parent
        color: Theme.veil
        opacity: player.transprant
    }

    // ===== 视频/图片预览（铺在右侧面板里，不是弹卡）=====
    VideoOutput
    {
        id: homeOutput
        anchors.fill: panelArea
        fillMode: VideoOutput.PreserveAspectCrop
        visible: homeRoot.imageORvideo === false
    }

    MediaPlayer
    {
        id: homeMedia
        videoOutput: homeOutput
        source: Theme.url(player.previewPath)
        autoPlay: true
        loops: MediaPlayer.Infinite
    }

    Image
    {
        anchors.fill: panelArea
        fillMode: Image.PreserveAspectCrop
        visible: homeRoot.imageORvideo === true
        source: Theme.url(player.previewPath)
    }

    // ===== 顶部：标题 / Tools / 处理进度 / 透明度 =====
    Item
    {
        id: barArea
        x: homeRoot.g.padX
        y: homeRoot.g.padT
        width: homeRoot.width - x - homeRoot.g.padX
        height: homeRoot.g.tabs === "barrow" ? 92 : 68

        Text
        {
            id: barTitle
            visible: !homeRoot.g.titleVertical
            text: "Music"
            // 固定占位：标题字号各方案不同，给固定宽度后 Tools 才不会跟着横向跳
            width: 132
            font.pixelSize: Math.min(homeRoot.g.titlePx, 32)
            font.bold: true
            font.family: homeRoot.g.titleSerif ? Theme.serif : Theme.family
            color: Theme.tx
            anchors.verticalCenter: parent.top
            anchors.verticalCenterOffset: 26
        }

        // Tools
        Text
        {
            id: toolsText
            text: "Tools ▼"
            property bool hovered: false
            font.pixelSize: 16
            font.family: Theme.family
            color: hovered ? Theme.tx : Theme.tx2
            anchors.left: barTitle.right
            anchors.leftMargin: 22
            anchors.verticalCenter: parent.top
            anchors.verticalCenterOffset: 26

            MouseArea
            {
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onEntered: toolsText.hovered = true
                onExited: toolsText.hovered = false
                onClicked: toolsMenu.popup()
            }

            Menu
            {
                id: toolsMenu

                background: Rectangle
                {
                    implicitWidth: 220
                    implicitHeight: 200
                    radius: Theme.r1
                    color: Theme.pan
                    border.color: Theme.brd
                    border.width: 1
                }

                MenuItem
                {
                    text: "添加音乐"
                    contentItem: Text
                    {
                        text: parent.text
                        color: Theme.tx
                        font.family: Theme.family
                        font.pixelSize: 14
                    }
                    onTriggered: musicDialog.open()
                }

                MenuItem
                {
                    text: "选择背景"
                    contentItem: Text
                    {
                        text: parent.text
                        color: Theme.tx
                        font.family: Theme.family
                        font.pixelSize: 14
                    }
                    onTriggered: imageDialog.open()
                }

                MenuItem
                {
                    text: "双耳分听"
                    contentItem: Text
                    {
                        text: parent.text
                        color: Theme.tx
                        font.family: Theme.family
                        font.pixelSize: 14
                    }
                    onTriggered:
                    {
                        binauralDialog.leftPath = ""
                        binauralDialog.leftName = ""
                        binauralDialog.reselect = false
                        binauralDialog.open()
                    }
                }

                MenuItem
                {
                    text: "歌词颜色"
                    contentItem: Text
                    {
                        text: parent.text
                        color: Theme.tx
                        font.family: Theme.family
                        font.pixelSize: 14
                    }
                    onTriggered: lrcColorDialog.open()
                }

                MenuItem
                {
                    text: "频谱颜色"
                    contentItem: Text
                    {
                        text: parent.text
                        color: Theme.tx
                        font.family: Theme.family
                        font.pixelSize: 14
                    }
                    onTriggered: spectrumColorDialog.open()
                }

                MenuItem
                {
                    text: "更换模型(.onnx文件)"
                    contentItem: Text
                    {
                        text: parent.text
                        color: Theme.tx
                        font.family: Theme.family
                        font.pixelSize: 14
                    }
                    onTriggered: modelDialog.open()
                }
            }
        }

        // 处理进度（music.taskName !== "" 才出现）
        Column
        {
            visible: music.taskName !== ""
            spacing: 5
            width: 380
            anchors.left: parent.left
            anchors.leftMargin: 260
            anchors.verticalCenter: homeRoot.g.tabs === "barrow" ? undefined : barTitle.verticalCenter
            anchors.bottom: homeRoot.g.tabs === "barrow" ? parent.bottom : undefined
            anchors.bottomMargin: 10

            Text
            {
                width: 380
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
                text: music.taskName
                color: Theme.tx
                font.pixelSize: 14
                font.family: Theme.family
            }

            Text
            {
                width: 380
                horizontalAlignment: Text.AlignHCenter
                text: "处理进度: " + music.value + "%"
                color: Theme.tx3
                font.pixelSize: 12
                font.family: Theme.family
            }

            Rectangle
            {
                width: 380
                height: 8
                radius: 4
                color: "transparent"
                border.color: Theme.brd
                border.width: 1

                Rectangle
                {
                    height: parent.height
                    width: parent.width * music.value / 100.0
                    radius: 4
                    color: Theme.acc

                    Behavior on width
                    {
                        NumberAnimation { duration: 300; easing.type: Easing.OutCubic }
                    }
                }
            }
        }

        // 背景透明度
        Row
        {
            anchors.right: parent.right
            anchors.verticalCenter: homeRoot.g.tabs === "barrow" ? undefined : barTitle.verticalCenter
            anchors.bottom: homeRoot.g.tabs === "barrow" ? parent.bottom : undefined
            anchors.bottomMargin: 16
            spacing: 10

            Text
            {
                anchors.verticalCenter: parent.verticalCenter
                text: "背景"
                color: Theme.tx3
                font.pixelSize: 12
                font.family: Theme.family
            }

            Slide10
            {
                width: 200
                anchors.verticalCenter: parent.verticalCenter
                from: 0
                to: 1
                value: player.transprant
                onMoved: player.transprant = value
            }
        }
    }

    // 竖排大标题（h5）
    Text
    {
        visible: homeRoot.g.titleVertical
        text: "Music"
        font.pixelSize: 56
        font.bold: true
        font.family: Theme.family
        color: Theme.tx
        rotation: 90
        anchors.horizontalCenter: railCol.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        width: 400
        horizontalAlignment: Text.AlignHCenter
    }

    // 竖排 tab 轨（h2）
    Item
    {
        id: railCol
        visible: homeRoot.g.tabs === "rail"
        x: homeRoot.g.padX
        width: homeRoot.g.railW
        y: homeRoot.g.padT + 90
        height: homeRoot.height - y - homeRoot.g.padB

        Column
        {
            anchors.centerIn: parent
            spacing: 10

            Repeater
            {
                model: ["音乐", "双耳", "图片"]
                delegate: TabBtn10
                {
                    vertical: true
                    active: homeRoot.panelMode === index
                    label: modelData
                    onClicked: homeRoot.pickTab(index)
                }
            }
        }
    }

    // 三张统计卡（h10）
    Column
    {
        visible: homeRoot.g.stats
        x: homeRoot.g.padX
        y: homeRoot.g.padT + 90
        spacing: 14
        width: homeRoot.g.railW

        Repeater
        {
            // 数字必须绑在 view 上：数组模型里的值是建好那一刻的快照，之后不跟随
            model: ["首音乐", "组双耳", "张背景"]
            delegate: Rectangle
            {
                width: homeRoot.g.railW
                height: 78
                radius: Theme.r2
                color: Theme.pan
                border.color: Theme.brd
                border.width: 1

                Text
                {
                    anchors.left: parent.left
                    anchors.leftMargin: 16
                    anchors.top: parent.top
                    anchors.topMargin: 14
                    text: index === 0 ? musicList.count : (index === 1 ? dblList.count : imgList.count)
                    color: Theme.tx
                    font.pixelSize: 26
                    font.bold: true
                    font.family: Theme.family
                }

                Text
                {
                    anchors.left: parent.left
                    anchors.leftMargin: 16
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 12
                    text: modelData
                    color: Theme.tx3
                    font.pixelSize: 11
                    font.family: Theme.family
                }
            }
        }
    }

    // ===== 列表区 =====
    Rectangle
    {
        id: listArea
        x: homeRoot.g.padX + (homeRoot.g.railW > 0 ? homeRoot.g.railW + 18 : 0)
        y: homeRoot.g.padT + barArea.height + (homeRoot.g.tabs === "barrow" ? 46 : 6)
        width: homeRoot.g.panelBottom
               ? homeRoot.width - homeRoot.g.padX * 2
               : homeRoot.width - homeRoot.g.padX * 2 - homeRoot.g.railW
                 - homeRoot.g.panelW - homeRoot.g.gap - (homeRoot.g.railW > 0 ? 18 : 0)
        height: homeRoot.g.panelBottom
                ? homeRoot.height - y - homeRoot.g.padB - 190 - 12
                : homeRoot.height - y - homeRoot.g.padB
        radius: homeRoot.g.bare ? 0 : Theme.r2
        color: homeRoot.g.bare ? "transparent" : Theme.pan
        border.color: homeRoot.g.bare ? "transparent" : Theme.brd
        border.width: homeRoot.g.bare ? 0 : 1
        clip: true

        // 表头
        Row
        {
            id: listHead
            visible: !homeRoot.gridMode
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 14
            spacing: 10
            height: 18
            z: 3

            Repeater
            {
                model: [
                    { "t": "#", "w": 34 },
                    { "t": "封面", "w": 44 },
                    { "t": "曲目", "w": -1 },
                    { "t": "备注", "w": 60 }
                ]
                delegate: Text
                {
                    text: modelData.t
                    color: Theme.tx3
                    font.pixelSize: 11
                    font.family: Theme.mono
                    width: modelData.w < 0 ? listHead.width - 160 : modelData.w
                    elide: Text.ElideRight
                }
            }
        }

        // 三个数据源 = 同一个列表换模型（他 Home.qml 的 panelMode）
        ListView
        {
            id: musicList
            visible: homeRoot.panelMode === 0 && !homeRoot.gridMode
            anchors.fill: parent
            anchors.topMargin: listHead.visible ? 40 : 12
            anchors.margins: 12
            model: music
            spacing: 5
            clip: true
            delegate: musicDelegate
        }

        ListView
        {
            id: dblList
            visible: homeRoot.panelMode === 1 && !homeRoot.gridMode
            anchors.fill: parent
            anchors.topMargin: listHead.visible ? 40 : 12
            anchors.margins: 12
            model: Dmusic
            spacing: 5
            clip: true
            delegate: dblDelegate
        }

        ListView
        {
            id: imgList
            visible: homeRoot.panelMode === 2 && !homeRoot.gridMode
            anchors.fill: parent
            anchors.topMargin: listHead.visible ? 40 : 12
            anchors.margins: 12
            model: image
            spacing: 5
            clip: true
            delegate: imgDelegate
        }

        GridView
        {
            id: musicGrid
            visible: homeRoot.panelMode === 0 && homeRoot.gridMode
            anchors.fill: parent
            anchors.margins: 12
            model: music
            cellWidth: homeRoot.cellW
            cellHeight: homeRoot.gridCardH
            clip: true
            delegate: musicDelegate
        }

        GridView
        {
            id: dblGrid
            visible: homeRoot.panelMode === 1 && homeRoot.gridMode
            anchors.fill: parent
            anchors.margins: 12
            model: Dmusic
            cellWidth: homeRoot.cellW
            cellHeight: homeRoot.gridCardH
            clip: true
            delegate: dblDelegate
        }

        GridView
        {
            id: imgGrid
            visible: homeRoot.panelMode === 2 && homeRoot.gridMode
            anchors.fill: parent
            anchors.margins: 12
            model: image
            cellWidth: homeRoot.cellW
            cellHeight: homeRoot.gridCardH
            clip: true
            delegate: imgDelegate
        }

        Text
        {
            anchors.centerIn: parent
            visible: homeRoot.curCount === 0
            text: homeRoot.panelMode === 2 ? "No Image" : "No Music"
            color: homeRoot.g.bare ? Theme.tx3 : Theme.tx2
            font.pixelSize: 15
            font.family: Theme.family
        }
    }

    // 模型是 QAbstractListModel，Qt 6.7 没给它 count 属性（Home.qml:511 那个 music.count 恒 undefined），
    // 所以只能问承载它的 ListView / GridView 要行数
    readonly property int curCount: panelMode === 0
                                    ? (gridMode ? musicGrid.count : musicList.count)
                                    : (panelMode === 1
                                       ? (gridMode ? dblGrid.count : dblList.count)
                                       : (gridMode ? imgGrid.count : imgList.count))

    // ===== 右侧预览面板 =====
    Item
    {
        id: panelArea
        x: homeRoot.g.panelBottom ? homeRoot.g.padX
                                  : listArea.x + listArea.width + homeRoot.g.gap
        y: homeRoot.g.panelBottom ? listArea.y + listArea.height + 12
                                  : listArea.y
        width: homeRoot.g.panelW
        height: homeRoot.g.panelBottom ? 190 : listArea.height
        z: 1

        Rectangle
        {
            anchors.fill: parent
            visible: !homeRoot.g.panelBordered
            radius: Theme.r3
            color: Theme.pan2
        }

        Rectangle
        {
            id: surface
            anchors.fill: parent
            anchors.margins: homeRoot.g.tabs === "rail" || homeRoot.g.tabs === "barrow" ? 0 : 0
            radius: homeRoot.g.panelBordered ? Theme.r2 : Theme.r3
            color: homeRoot.g.panelBordered ? Theme.pan : "transparent"
            border.color: homeRoot.g.panelBordered ? Theme.brd : "transparent"
            border.width: homeRoot.g.panelBordered ? 1 : 0
            clip: true
        }
    }

    // 预览提示（没悬停时）
    Column
    {
        visible: homeRoot.pendingPath === "" && !homeRoot.imageORvideo
        anchors.centerIn: panelArea
        spacing: 6

        Text
        {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "悬停列表项 → 面板预览"
            color: Theme.tx3
            font.pixelSize: 13
            font.family: Theme.family
        }

        Text
        {
            anchors.horizontalCenter: parent.horizontalCenter
            width: panelArea.width - 40
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            text: "音乐 0.8s 走 video/<歌名>.mp4，图片 0.5s 走 Image"
            color: Theme.tx3
            opacity: 0.75
            font.pixelSize: 11
            font.family: Theme.family
        }
    }

    Rectangle
    {
        visible: homeRoot.pendingPath !== ""
        x: panelArea.x + 12
        y: panelArea.y + 12
        width: tagTxt.implicitWidth + 20
        height: 22
        radius: 11
        color: "#73000000"

        Text
        {
            id: tagTxt
            anchors.centerIn: parent
            text: String(homeRoot.pendingPath).split(/[\/]/).pop()
            color: "white"
            font.pixelSize: 11
            font.family: Theme.family
        }
    }

    // ===== tab =====
    function pickTab(k)
    {
        panelMode = k
        player.setPanelmodel(k)
        if (k === 0)
        {
            player.setNormMusiclist(player.getAllPaths())
            Dplay.stopRightMusic()
            Dplay.stopLeftMusic()
        }
        else
        {
            player.Pause()
        }
    }

    Row
    {
        visible: homeRoot.g.tabs === "strip" || homeRoot.g.tabs === "column"
                 || homeRoot.g.tabs === "pills" || homeRoot.g.tabs === "barrow"
        spacing: 4
        x: homeRoot.g.tabs === "barrow" ? barArea.x
           : (homeRoot.g.tabs === "pills" ? panelArea.x + panelArea.width - tabsRow.implicitWidth - 12
                                         : panelArea.x + 12)
        y: homeRoot.g.tabs === "barrow" ? barArea.y + barArea.height - 40
           : (homeRoot.g.tabs === "pills" ? panelArea.y + 12 : panelArea.y + 8)
        id: tabsRow

        Repeater
        {
            model: ["音乐列表", "双耳分听", "背景图片"]
            delegate: TabBtn10
            {
                vertical: homeRoot.g.tabs === "column"
                pills: homeRoot.g.tabs === "pills" || homeRoot.g.tabs === "barrow"
                active: homeRoot.panelMode === index
                label: modelData
                onClicked: homeRoot.pickTab(index)
            }
        }
    }

    // 面板底部说明
    Rectangle
    {
        visible: !homeRoot.g.panelBordered
        anchors.left: panelArea.left
        anchors.right: panelArea.right
        anchors.bottom: panelArea.bottom
        height: 30
        color: "#4D000000"

        Text
        {
            anchors.centerIn: parent
            text: Theme.n + " / 10 · " + Theme.name + "（数字键切套）"
            color: Theme.tx3
            font.pixelSize: 10
            font.family: Theme.family
        }
    }

    // ===== 委托 =====
    Component
    {
        id: musicDelegate

        Row10
        {
            width: homeRoot.rowW
            idx: index + 1
            title: musicName
            sub: musicSinger
            cover: Theme.url(musicCover)
            meta: ""
            rowH: homeRoot.g.rowH
            cvPx: homeRoot.gridMode ? homeRoot.cellW - 20 : homeRoot.g.cvPx
            gridCard: homeRoot.gridMode
            hairline: homeRoot.g.hairline
            idxSerif: homeRoot.g.idxSerif
            squareCover: homeRoot.g.squareCv
            titlePx: homeRoot.gridMode ? 13 : (homeRoot.g.rowH < 44 ? 12.5 : 15)
            selected: player.currentMusicName === musicName
            onEnter: homeRoot.beginPreview(0, musicPath)
            onExit: homeRoot.endPreview(0)
            onClickedLeft:
            {
                homeRoot.panelMode = 0
                stack.push("Play.qml")
                player.playMusic(musicPath)
                player.playVideo(musicPath)
                audioAnalyzer.loadMusic(musicPath)
            }
            onClickedRight:
            {
                homeRoot.detIndex = index
                musicMenu.musicName = musicName
                musicMenu.musicPath = musicPath
                musicMenu.popup()
            }
        }
    }

    Component
    {
        id: dblDelegate

        Row10
        {
            width: homeRoot.rowW
            idx: 0
            title: musicName
            sub: musicSinger
            meta: "L·R"
            rowH: homeRoot.g.rowH
            cvPx: homeRoot.gridMode ? homeRoot.cellW - 20 : homeRoot.g.cvPx
            gridCard: homeRoot.gridMode
            hairline: homeRoot.g.hairline
            titlePx: homeRoot.gridMode ? 13 : (homeRoot.g.rowH < 44 ? 12.5 : 15)
            onEnter: homeRoot.beginPreview(1, musicPath)
            onExit: homeRoot.endPreview(1)
            onClickedLeft:
            {
                stack.push("PlayDouble.qml")
                Dplay.loadDoubleMusic(musicPath)
            }
            onClickedRight:
            {
                homeRoot.detIndex = index
                musicMenu.musicName = musicName
                musicMenu.musicPath = musicPath
                musicMenu.popup()
            }
        }
    }

    Component
    {
        id: imgDelegate

        Row10
        {
            width: homeRoot.rowW
            idx: 0
            title: imageName
            meta: "设为背景"
            cover: Theme.url(imagePath)
            cvR: 8
            rowH: homeRoot.g.rowH
            cvPx: homeRoot.gridMode ? homeRoot.cellW - 20 : homeRoot.g.cvPx
            gridCard: homeRoot.gridMode
            hairline: homeRoot.g.hairline
            titlePx: homeRoot.gridMode ? 13 : (homeRoot.g.rowH < 44 ? 12.5 : 15)
            onEnter: homeRoot.beginPreview(2, imagePath)
            onExit: homeRoot.endPreview(2)
            onClickedLeft: player.setbgImage(imagePath)
            onClickedRight:
            {
                homeRoot.detIndex = index
                musicMenu.popup()
            }
        }
    }

    // ===== 右键菜单 =====
    Menu
    {
        id: musicMenu

        property string musicName: ""
        property string musicPath: ""

        background: Rectangle
        {
            implicitWidth: 170
            implicitHeight: 252
            radius: Theme.r1
            color: Theme.pan
            border.color: Theme.brd
            border.width: 1
        }

        MenuItem
        {
            text: "删除"
            contentItem: Text { text: parent.text; color: Theme.tx; font.family: Theme.family }
            onTriggered:
            {
                player.stopMusic()
                if (homeRoot.panelMode === 0)
                    music.removeMusic(homeRoot.detIndex)
                else if (homeRoot.panelMode === 1)
                    Dmusic.removeMusic(homeRoot.detIndex)
                else
                {
                    image.removeImage(homeRoot.detIndex)
                    player.setbgImage()
                }
            }
        }

        MenuItem
        {
            text: "添加视频"
            visible: homeRoot.panelMode === 0
            contentItem: Text { text: parent.text; color: Theme.tx; font.family: Theme.family }
            onTriggered:
            {
                player.Pause()
                videoDialog.open()
            }
        }

        MenuItem
        {
            text: "添加歌词"
            visible: homeRoot.panelMode === 0
            contentItem: Text { text: parent.text; color: Theme.tx; font.family: Theme.family }
            onTriggered:
            {
                player.Pause()
                lrcDialog.open()
            }
        }

        MenuItem
        {
            text: "添加封面"
            visible: homeRoot.panelMode === 0
            contentItem: Text { text: parent.text; color: Theme.tx; font.family: Theme.family }
            onTriggered:
            {
                player.Pause()
                coverDialog.open()
            }
        }

        MenuItem
        {
            text: "删除视频"
            visible: homeRoot.panelMode === 0
            contentItem: Text { text: parent.text; color: Theme.tx; font.family: Theme.family }
            onTriggered:
            {
                player.Pause()
                music.removeVideo(homeRoot.detIndex)
            }
        }

        MenuItem
        {
            text: "检查并处理"
            visible: homeRoot.panelMode === 0
            contentItem: Text { text: parent.text; color: Theme.tx; font.family: Theme.family }
            onTriggered:
            {
                player.Pause()
                processDialog.musicPath = musicMenu.musicPath
                processDialog.open()
            }
        }
    }

    // ===== 文件 / 颜色对话框（和他 Home.qml 一一对应）=====
    FileDialog
    {
        id: videoDialog
        title: "选择视频"
        nameFilters: ["Video Files (*.mp4 *.mkv *.avi)"]
        onAccepted: video.addVideo(currentFile, musicMenu.musicName)
    }

    FileDialog
    {
        id: imageDialog
        title: "选择背景图片"
        nameFilters: ["image Files (*.jpg *.png)"]
        onAccepted: player.addbgImage(currentFile)
    }

    FileDialog
    {
        id: musicDialog
        title: "选择音乐"
        nameFilters: ["Audio Files (*.mp3 *.wav *.flac)"]
        onAccepted: music.addMusic(currentFile)
    }

    FileDialog
    {
        id: binauralDialog

        nameFilters: ["Audio Files (*.mp3 *.wav *.flac)"]
        fileMode: FileDialog.OpenFile
        property string leftPath: ""
        property string leftName: ""
        property bool reselect: false

        title: leftPath === "" ? "① 选择左耳音乐" : "② 选择右耳音乐（左耳：" + leftName + "）"

        function toLocalPath(u)
        {
            var s = u.toString()
            if (s.indexOf("file:///") === 0)
                s = s.substring(8)
            else if (s.indexOf("file://") === 0)
                s = s.substring(7)
            try { s = decodeURIComponent(s) } catch (e) {}
            return s.toLowerCase()
        }

        function shortName(u)
        {
            var s = u.toString()
            var i = Math.max(s.lastIndexOf("/"), s.lastIndexOf("\\"))
            if (i >= 0)
                s = s.substring(i + 1)
            try { s = decodeURIComponent(s) } catch (e) {}
            return s
        }

        onAccepted:
        {
            if (currentFile.toString().length === 0)
                return

            if (leftPath === "")
            {
                leftPath = currentFile.toString()
                leftName = shortName(currentFile)
                Qt.callLater(binauralDialog.open)
                return
            }

            if (toLocalPath(currentFile) === toLocalPath(leftPath))
            {
                reselect = true
                repeatDialog.text = "左右耳不能选择同一首音乐"
                repeatDialog.open()
                return
            }

            music.playDimensionalMusic(leftPath, currentFile.toString())
            leftPath = ""
            leftName = ""
        }
    }

    FileDialog
    {
        id: lrcDialog
        title: "选择歌词文件"
        nameFilters: ["LRC歌词 (*.lrc)", "QRC歌词 (*.qrc)", "KRC歌词 (*.krc)"]
        onAccepted: music.addMusicLrc(currentFile, musicMenu.musicName)
    }

    FileDialog
    {
        id: coverDialog
        title: "选择封面文件"
        nameFilters: ["jpg (*.jpg)", "png (*.png)"]
        onAccepted: music.addMusicCover(currentFile, musicMenu.musicPath)
    }

    FileDialog
    {
        id: modelDialog
        title: "选择模型(onnx)文件"
        nameFilters: ["onnx(*.onnx)"]
        onAccepted: music.replaceModel(currentFile)
    }

    MessageDialog
    {
        id: repeatDialog

        title: "提示"
        text: ""

        onAccepted:
        {
            if (binauralDialog.reselect)
            {
                binauralDialog.reselect = false
                Qt.callLater(binauralDialog.open)
            }
        }
    }

    MessageDialog
    {
        id: processDialog

        title: "检查并处理"

        property string musicPath: ""

        text: "将检查该歌曲并生成缺失的处理产物：\n· 环绕\n· 人声 / 伴奏\n\n已存在的不会重做，现在开始？"

        buttons: MessageDialog.Yes | MessageDialog.No

        onAccepted: music.checkFile(processDialog.musicPath)
    }

    Connections
    {
        target: video

        function onAddVideoFailed(msg)
        {
            repeatDialog.text = msg
            repeatDialog.open()
        }
    }

    ColorDialog
    {
        id: lrcColorDialog

        title: "选择歌词颜色"

        onAccepted: player.lrcColor = selectedColor.toString()
    }

    ColorDialog
    {
        id: spectrumColorDialog

        title: "选择频谱颜色"

        onAccepted: player.spectrumColor = selectedColor.toString()
    }

    DropArea
    {
        anchors.fill: parent
        keys: ["text/uri-list"]
        visible: false

        onDropped:
        {
            var urls = drop.urls
            for (var i = 0; i < urls.length; ++i)
            {
                var path = urls[i].toString()
                if (path.startsWith("file:///"))
                    path = path.substring(8)
                var ext = path.split(".").pop().toLowerCase()
                homeRoot.handleFile(path, ext)
            }
        }
    }

    function handleFile(path, ext)
    {
        if (["mp3", "wav", "flac"].indexOf(ext) !== -1)
            music.addMusic(path)
        else if (["jpg", "jpeg", "png", "bmp"].indexOf(ext) !== -1)
            player.setbgImage(path)
        else if (ext === "onnx")
            music.replaceModel(path)
        else
            console.log("不支持的文件类型:", ext)
    }
}
