import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtMultimedia
import Qt5Compat.GraphicalEffects

Item
{
    id:homeRoot
    property var stack
    //复制切换列表数据源
    property int panelMode: 0
    property bool imageORvideo:false
    Component.onCompleted: 
    {
        stack = StackView.view
        player.setbgImage()
    }
    //提示弹窗
    Connections
    {
        target:video
        function onAddVideoFailed(msg)
        {
            repeatDialog.text = msg
            repeatDialog.open()
        }
    }
    MessageDialog
    {
        id: repeatDialog

        title: "提示"

        text: ""
    }

    // 背景图
    Image
    {
        anchors.fill: parent

        source: "file:///" + player.bgPath

        fillMode: Image.PreserveAspectCrop
    }

    //视频预览播放器
    VideoOutput
    {
        id:musicOutput
        anchors.fill:videoPanel
        fillMode: VideoOutput.PreserveAspectCrop
        visible:homeRoot.imageORvideo === false
    }
    MediaPlayer
    {
        id:videoplayer
        videoOutput:musicOutput
        source:player.previewPath
        autoPlay:true
        loops: MediaPlayer.Infinite
    }

    //图片预览
    Image
    {
        id:previewImage

        anchors.fill:videoPanel

        fillMode:Image.PreserveAspectCrop

        visible: homeRoot.imageORvideo === true

        source:"file:///" + player.previewPath
    }
    //暗层
    Rectangle
    {
        anchors.fill: parent
        color: "black"
        opacity: opacitySlider.value
    }

    //菜单栏
    Column
    {
        id:topMenu

        anchors.top: parent.top
        anchors.left: parent.left

        anchors.topMargin: 25
        anchors.leftMargin: 35

        spacing: 8

        Text//标题
        {
            id: title

            text: "Music"

            font.pixelSize: 30
            font.bold: true
            color: "white"
        }

        Text//Tools
        {
            id: toolsText

            text: "Tools ▼"

            font.pixelSize: 16
            color: hovered ? "white" : "#CCCCCC"

            property bool hovered: false

            MouseArea
            {
                anchors.fill: parent

                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor

                onEntered: toolsText.hovered = true
                onExited: toolsText.hovered = false

                onClicked:
                {
                    toolsMenu.popup()
                }
            }
        }

        Menu
        {
            id: toolsMenu

            background: Rectangle
            {
                implicitWidth: 220
                implicitHeight: 80

                radius: 10
                color: "#AA333333"
                border.color: "#666666"
            }

            MenuItem
            {
                text: "添加音乐"
                onTriggered: musicDialog.open()
            }

            MenuSeparator { }

            MenuItem
            {
                text: "选择背景"
                onTriggered: imageDialog.open()
            }

            MenuItem
            {
                text:"双耳分听"
                onTriggered:binauralDialog.open()
            }

            MenuItem
            {
                text:"歌词颜色"
                onTriggered:
                {
                    lrcColorDialog.open()
                }
            }
            MenuItem
            {
                text:"频谱颜色"
                onTriggered:
                {
                    spectrumColorDialog.open()
                }
            }
            MenuItem
            {
                text:"更换模型(.onnx文件)"
                onTriggered:
                {
                    spectrumColorDialog.open()
                }
            }
        }
    }

    //处理进度条
    Column
    {
        id: taskProgress

        visible:music.taskName !== ""

        anchors.left: topMenu.right
        anchors.leftMargin:80

        anchors.verticalCenter: topMenu.verticalCenter

        spacing:5

        Text
        {
            text: music.taskName

            color:"white"

            font.pixelSize:16

            width:380

            horizontalAlignment: Text.AlignHCenter

            elide: Text.ElideRight
        }

        Text
        {
            text: "处理进度: " + music.value + "%"

            color:"#CCCCCC"

            font.pixelSize:12

            width:380

            horizontalAlignment: Text.AlignHCenter
        }

        // 自定义进度条
        Rectangle
        {
            id: progressBar

            width:380
            height:8

            radius:4

            color:"transparent"

            border.color:"#888888"
            border.width:1


            Rectangle
            {
                id: progressFill

                height:parent.height

                width: parent.width * music.value / 100.0

                radius:4

                color:"#3A7AFE"


                Behavior on width
                {
                    NumberAnimation
                    {
                        duration:300

                        easing.type:Easing.OutCubic
                    }
                }
            }
        }
    }

    //背景图透明度调节
    Slider
    {
        id: opacitySlider

        width: parent.width / 5

        anchors.top: parent.top
        anchors.right: parent.right

        anchors.topMargin: 35
        anchors.rightMargin: 35

        opacity:0.6

        from: 0.0
        to: 1.0

        value: player.transprant

        onMoved: player.transprant = value
    }

    // 音乐列表
    Rectangle
    {
        id: listPanel

        width:parent.width / 2

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left:parent.left

        anchors.topMargin: 100
        anchors.margins: 20

        radius: 20

        color: "white"

        opacity: 0.3

        border.color: "white"

        property int detIndex: -1
        
        //音乐
        ListView
        {
            id: musicListView

            visible:homeRoot.panelMode === 0

            anchors.fill: parent

            anchors.margins: 15

            spacing: 5

            clip: true

            model: music

            delegate: Rectangle
            {
                width: musicListView.width
                height: 58

                radius: 10

                color: hovered ? "lightgray" : "transparent"

                property bool hovered: false

                Behavior on color
                {
                    ColorAnimation
                    {
                        duration: 120
                    }
                }

                MouseArea
                {
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton | Qt.RightButton

                    Timer
                    {
                        id:hoverTimer

                        interval:800

                        repeat:false

                        onTriggered:
                        {
                            homeRoot.imageORvideo = false
                            player.playPreviewMusic(musicPath)
                        }
                    }

                    Timer
                    {
                        id:stopTimer

                        interval:500

                        repeat:false

                        onTriggered:
                        {
                            homeRoot.imageORvideo = false
                            player.playPreviewMusic("")
                        }
                    }

                    onEntered:
                    {
                        parent.hovered = true
                        stopTimer.stop()
                        hoverTimer.start()
                    }

                    onExited:
                    {
                        parent.hovered = false
                        hoverTimer.stop()
                        stopTimer.start()                
                    }

                    //右键菜单
                    onClicked:function(mouse)
                    {
                        if(mouse.button === Qt.RightButton)
                        {
                            listPanel.detIndex = index
                            musicMenu.musicName = musicName
                            musicMenu.musicPath = musicPath
                            musicMenu.popup()
                        }
                        if(mouse.button === Qt.LeftButton)
                        {
                            stack.push("Play.qml")
                            player.playMusic(musicPath)
                            console.log("musicPath:" + musicPath)
                            player.playVideo(musicPath)
                            audioAnalyzer.loadMusic(musicPath)
                        }
                    }
                }
                Row
                {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 15
                    Image
                    {
                        id:coverImage
                        width: 50
                        height: 50

                        source: musicCover

                        fillMode: Image.PreserveAspectCrop

                        layer.enabled: true
                    }
                    
                    Column
                    {
                        anchors.verticalCenter: parent.verticalCenter

                        spacing: 4

                        // 歌名
                        Text 
                        {
                            text: musicName
                            color: "black"
                            font.bold: true
                            font.pixelSize: 16

                            layer.enabled: true
                            layer.effect: DropShadow 
                            {
                                horizontalOffset: 1
                                verticalOffset: 1
                                radius: 4
                                samples: 8
                                color: "#80000000"
                            }
                        }

                        // 歌手
                        Text
                        {
                            text: musicSinger

                            font.pixelSize: 12

                            color: "black"

                            elide: Text.ElideRight
                        }
                    }
                }
            }

            // 空列表提示
            Text
            {
                visible: music.count === 0

                text: "No Music"

                anchors.centerIn: parent

                color: "white"

                font.pixelSize: 16
            } 
        }
        //dmusic
        ListView
        {
            id: dmusicListView

            visible:homeRoot.panelMode === 1

            anchors.fill: parent

            anchors.margins: 15

            spacing: 5

            clip: true

            model: Dmusic

            delegate: Rectangle
            {
                width: musicListView.width
                height: 58

                radius: 10

                color: hovered ? "lightgray" : "transparent"

                property bool hovered: false

                Behavior on color
                {
                    ColorAnimation
                    {
                        duration: 120
                    }
                }

                MouseArea
                {
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton | Qt.RightButton

                     Timer
                    {
                        id:hoverTimerDmusic

                        interval:500

                        repeat:false

                        onTriggered:
                        {
                            player.playPreviewMusic(musicPath)
                        }
                    }

                    onEntered:
                    {
                        parent.hovered = true

                        hoverTimerDmusic.start()
                    }

                    onExited:
                    {
                        parent.hovered = false
                        player.playPreviewMusic("")
                        hoverTimerDmusic.stop()
                    }

                    onClicked:function(mouse)
                    {
                        if(mouse.button === Qt.RightButton)
                        {
                            listPanel.detIndex = index
                            removeMusic.popup()
                        }
                        if(mouse.button === Qt.LeftButton)
                        {
                            stack.push("PlayDouble.qml")
                            Dplay.loadDoubleMusic(musicPath)
                        }
                    }
                }

                Column
                {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left

                    anchors.leftMargin: 15

                    spacing: 4

                    // 歌名
                    Text 
                    {
                        text: musicName
                        color: "black"
                        font.bold: true
                        font.pixelSize: 16

                        layer.enabled: true
                        layer.effect: DropShadow 
                        {
                            horizontalOffset: 1
                            verticalOffset: 1
                            radius: 4
                            samples: 8
                            color: "#80000000"
                        }
                    }

                    // 歌手
                    Text
                    {
                        text: musicSinger

                        font.pixelSize: 12

                        color: "black"

                        elide: Text.ElideRight
                    }
                }
            }

            // 空列表提示
            Text
            {
                visible: music.count === 0

                text: "No Music"

                anchors.centerIn: parent

                color: "white"

                font.pixelSize: 16
            } 
        }
        //图片
        ListView
        {
            id: bgimgListView
    
            visible:homeRoot.panelMode === 2

            anchors.fill: parent

            anchors.margins: 15

            spacing: 5

            clip: true

            model: image

            delegate: Rectangle
            {
                width: musicListView.width
                height: 58

                radius: 10

                color: hovered ? "lightgray" : "transparent"

                property bool hovered: false

                Behavior on color
                {
                    ColorAnimation
                    {
                        duration: 120
                    }
                }

                MouseArea
                {
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton | Qt.RightButton

                    Timer
                    {
                        id:hoverTimerimg

                        interval:500

                        repeat:false

                        onTriggered:
                        {
                            player.playPreviewImage(imagePath)
                            homeRoot.imageORvideo = true
                        }
                    }

                    onEntered:
                    {
                        parent.hovered = true

                        hoverTimerimg.start()
                    }

                    onExited:
                    {
                        parent.hovered = false
                        player.playPreviewImage("")
                        hoverTimerimg.stop()
                    }

                    //右键菜单
                    onClicked:function(mouse)
                    {
                        if(mouse.button === Qt.RightButton)
                        {
                            //这里右键删除图片
                            listPanel.detIndex = index
                            removeMusic.popup()
                        }
                        if(mouse.button === Qt.LeftButton)
                        {
                            //左键设置图片为背景
                            player.setbgImage(imagePath)
                        }
                    }
                }

                Column
                {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left

                    anchors.leftMargin: 15

                    spacing: 4

                    Text 
                    {
                        text: imageName
                        color: "black"
                        font.bold: true
                        font.pixelSize: 16

                        layer.enabled: true
                        layer.effect: DropShadow 
                        {
                            horizontalOffset: 1
                            verticalOffset: 1
                            radius: 4
                            samples: 8
                            color: "#80000000"
                        }
                    }
                }
            }

            // 空列表提示
            Text
            {
                visible: music.count === 0

                text: "No Music"

                anchors.centerIn: parent

                color: "white"

                font.pixelSize: 16
            } 
        }
    }

    //资源列表
    Rectangle
    {
        id: videoPanel

        width: parent.width / 2 - 30

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right

        anchors.topMargin: 100
        anchors.margins: 20

        radius: 20
        color: "white"
        opacity: 0.3

        Column
        {
            width: parent.width
            spacing: 5
        
            //音乐
            Rectangle
            {
                id: musicTab
                width: parent.width
                height: 58
                radius: 12

                // ✅ 选中时高亮
                color: {
                    if (homeRoot.panelMode === 0) {
                        return "#3A7AFE"  // 选中时亮蓝色
                    } else if (hovered) {
                        return "#55FFFFFF"
                    } else {
                        return "#33000000"
                    }
                }

                border.color: homeRoot.panelMode === 0 ? "#3A7AFE" : "#66FFFFFF"
                border.width: 1

                property bool hovered: false

                Behavior on color {
                    ColorAnimation { duration: 150 }
                }

                MouseArea
                {
                    anchors.fill: parent
                    hoverEnabled: true

                    onEntered: parent.hovered = true
                    onExited: parent.hovered = false

                    onClicked:
                    {
                        homeRoot.panelMode = 0
                        player.setPanelmodel(0);
                        player.setNormMusiclist(player.getAllPaths())
                        Dplay.stopRightMusic()
                        Dplay.stopLeftMusic()
                    }
                }

                Text
                {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 15

                    text: "音乐列表"

                    // ✅ 选中时文字变白
                    color: homeRoot.panelMode === 0 ? "white" : "black"

                    font.pixelSize: 17
                    font.bold: true
                }
            }
        
            //双耳
            Rectangle
            {
                id: doubleTab
                width: parent.width
                height: 58
                radius: 12

                // ✅ 选中时高亮
                color: {
                    if (homeRoot.panelMode === 1) {
                        return "#3A7AFE"
                    } else if (hovered) {
                        return "#55FFFFFF"
                    } else {
                        return "#33000000"
                    }
                }

                border.color: homeRoot.panelMode === 1 ? "#3A7AFE" : "#66FFFFFF"
                border.width: 1

                property bool hovered: false

                Behavior on color {
                    ColorAnimation { duration: 150 }
                }

                MouseArea
                {
                    anchors.fill: parent
                    hoverEnabled: true

                    onEntered: parent.hovered = true
                    onExited: parent.hovered = false

                    onClicked:
                    {
                        homeRoot.panelMode = 1
                        player.setPanelmodel(1);
                        player.Pause()
                    }
                }

                Text
                {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 15

                    text: "双耳分听"

                    // ✅ 选中时文字变白
                    color: homeRoot.panelMode === 1 ? "white" : "black"

                    font.pixelSize: 17
                    font.bold: true
                }
            }
        
            //图片
            Rectangle
            {
                id: imageTab
                width: parent.width
                height: 58
                radius: 10

                // ✅ 选中时高亮
                color: {
                    if (homeRoot.panelMode === 2) {
                        return "#3A7AFE"
                    } else if (hovered) {
                        return "lightgray"
                    } else {
                        return "transparent"
                    }
                }

                property bool hovered: false

                Behavior on color {
                    ColorAnimation { duration: 120 }
                }

                MouseArea
                {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor

                    onEntered: parent.hovered = true
                    onExited: parent.hovered = false

                    onClicked:
                    {
                        homeRoot.panelMode = 2
                        player.setPanelmodel(2);
                        player.Pause()
                    }
                }

                Text
                {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 15

                    text: "背景图片"

                    font.pixelSize: 16
                    font.bold: true

                    // ✅ 选中时文字变白
                    color: homeRoot.panelMode === 2 ? "white" : "black"
                }
            }
        }
    }
    //资源添加：
    //视频
    FileDialog 
    {
        id: videoDialog
        title: "选择视频"
        nameFilters: ["Video Files (*.mp4 *.mkv *.avi)"]
        property string selectedVideoPath:""
        onAccepted: 
        {
            selectedVideoPath = videoDialog.currentFile
            video.addVideo(selectedVideoPath,musicMenu.musicName)
        }
    }
    //背景
    FileDialog 
    {
        id: imageDialog
        title: "选择背景图片"
        nameFilters: ["image Files (*.jpg *.png)"]

        onAccepted: 
        {
            player.addbgImage(imageDialog.currentFile)
        }
    }
    //音乐
    FileDialog 
    {
        id: musicDialog
        title: "选择音乐"
        nameFilters: ["Video Files (*.mp3 *.wav *.flac)"]

        onAccepted: 
        {
            music.addMusic(musicDialog.currentFile)
        }
    }
    //双耳
    FileDialog
    {
        id: binauralDialog

        title: "选择两首音乐（第一首左耳，第二首右耳）"

        nameFilters:
        [
            "Audio Files (*.mp3 *.wav *.flac)"
        ]

        fileMode: FileDialog.OpenFiles

        onAccepted:
        {
            console.log(selectedFiles)

            if(selectedFiles.length !== 2)
            {
                console.log("请选择两首音乐")
                return
            }

            var leftMusic = selectedFiles[0]

            var rightMusic = selectedFiles[1]

            music.playDimensionalMusic(
                leftMusic,
                rightMusic
            )
        }
    }

    //选择歌词文件
    FileDialog
    {
        id: lrcDialog

        title: "选择歌词文件"

        nameFilters:
        [
            "LRC歌词 (*.lrc)",
            "QRC歌词 (*.qrc)",
            "KRC歌词 (*.krc)"
        ]

        property string selectedLrcPath:""

        onAccepted:
        {
            selectedLrcPath = currentFile
            music.addMusicLrc(selectedLrcPath,musicMenu.musicName)
        }
    }
    
    //封面
    FileDialog
    {
        id: coverDialog

        title: "选择封面文件"

        nameFilters:
        [
            "jpg (*.jpg)",
            "png (*.png)"
        ]

        property string selectedCoverPath:""

        onAccepted:
        {
            selectedCoverPath = currentFile
            music.addMusicCover(selectedCoverPath,musicMenu.musicPath)
        }
    }

    //模型
    FileDialog
    {
        id: modelDialog

        title: "选择模型(onnx)文件"

        nameFilters:
        [
            "onnx(*.onnx)"
        ]

        property string selectedModelPath:""

        onAccepted:
        {
            selectedModelPath = currentFile
            music.replaceModel(selectedModelPath)
        }
    }

    //删除视频
    Menu
    {
        id: removeMenu

        MenuItem
        {
            text: "删除"

            onTriggered:
            {
                video.removeVideo(videoPanel.detIndex)
            }
        }
    }
    //右键歌曲功能
    Menu
    {
        id: musicMenu

        property string musicName:""
        property string musicPath:""
        //删除
        MenuItem
        {
            text: "删除"

            onTriggered:
            {
                player.stopMusic()
                if(homeRoot.panelMode === 0)
                    music.removeMusic(listPanel.detIndex)
                else if(homeRoot.panelMode === 1)
                    Dmusic.removeMusic(listPanel.detIndex)
                else if(homeRoot.panelMode === 2)
                    image.removeImage(listPanel.detIndex)
            }
        }
        //添加视频
        MenuItem
        {
            text: "添加视频"

            onTriggered:
            {
                player.Pause()
                if(homeRoot.panelMode === 0)
                {
                    videoDialog.open()
                }
            }
        }
        //添加歌词
        MenuItem
        {
            text: "添加歌词"

            onTriggered:
            {
                player.Pause()
                if(homeRoot.panelMode === 0)
                {
                    lrcDialog.open()
                }
            }
        }
        //添加封面
        MenuItem
        {
            text: "添加封面"

            onTriggered:
            {
                player.Pause()
                if(homeRoot.panelMode === 0)
                {
                    coverDialog.open()
                }
            }
        }
    }
    //颜色
    //歌词
    ColorDialog
    {
        id:lrcColorDialog

        title:"选择歌词颜色"

        onAccepted:
        {
            player.lrcColor = selectedColor.toString()
        }
    }
    //频谱
    ColorDialog
    {
        id:spectrumColorDialog

        title:"选择频谱颜色"

        onAccepted:
        {
            player.spectrumColor = selectedColor.toString()
        }
    }
    //拖放
    DropArea 
    {
        anchors.fill: parent
        keys: ["text/uri-list"]
        visible:false

        onEntered:
        {
        console.log("进入")
        }

        onDropped: 
        {
            // 接受拖放
            // 解析文件路径
            var urls = drop.urls
            for (var i = 0; i < urls.length; ++i) 
            {
                var url = urls[i]
                var path = url.toString()
                if (path.startsWith("file:///")) 
                {
                    path = path.substring(8) 
                }
                // 获取文件扩展名
                var extension = path.split('.').pop().toLowerCase()
                handleFile(path, extension)
            }
                    console.log("释放")

            console.log(drop.urls)
        }

        function handleFile(path, ext) 
        {
            // 音乐文件
            if (["mp3", "wav", "flac"].indexOf(ext) !== -1)
            {
                music.addMusic(path)
            }
            // 图片文件（背景）
            else if (["jpg", "jpeg", "png", "bmp"].indexOf(ext) !== -1) 
            {
                player.setbgImage(path)
            }
            else if(["onnx"])
            {
                music.replaceModel(path)
            }
            else 
            {
                console.log("不支持的文件类型:", ext)
            }
        }
    }
}