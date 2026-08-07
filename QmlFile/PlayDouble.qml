import QtQuick
import QtQuick.Controls
import QtMultimedia
import Qt5Compat.GraphicalEffects

Item
{
    anchors.fill: parent

    property int leftLrcIndex: -1
    property int rightLrcIndex: -1

    // ===== 自适应尺寸 =====
    property real baseUnit: Math.min(width, height)
    property real coverSize: Math.max(150, Math.min(280, baseUnit * 0.36))
    property real playerColWidth: coverSize + baseUnit * 0.05
    property real lrcColWidth: Math.max(140, Math.min(300, width * 0.2))
    property real lrcHeight: Math.max(320, Math.min(520, height * 0.72))
    property real mainSpacing: Math.max(6, Math.min(20, width * 0.012))
    property real colSpacing: Math.max(8, Math.min(15, baseUnit * 0.02))
    property real volSliderWidth: Math.max(8, Math.min(12, baseUnit * 0.015))
    property real titleFontSize: Math.max(16, Math.min(26, baseUnit * 0.033))
    property real songFontSize: Math.max(14, Math.min(22, baseUnit * 0.028))
    property real lrcHighlightFont: Math.max(16, Math.min(26, baseUnit * 0.032))
    property real lrcNormalFont: Math.max(12, Math.min(20, baseUnit * 0.024))
    property real timeFontSize: Math.max(10, Math.min(14, baseUnit * 0.016))

    // 辅助函数：格式化时间（毫秒 -> mm:ss）
    function formatTime(ms)
    {
        if (ms <= 0) return "00:00"
        var seconds = Math.floor(ms / 1000)
        var minutes = Math.floor(seconds / 60)
        seconds = seconds % 60
        return (minutes < 10 ? "0" + minutes : minutes) + ":" + (seconds < 10 ? "0" + seconds : seconds)
    }

    // 辅助函数：歌词滚动
    onLeftLrcIndexChanged:
    {
        if (leftLrcIndex >= 0)
        {
            leftListView.positionViewAtIndex(leftLrcIndex, ListView.Center)
        }
    }

    onRightLrcIndexChanged:
    {
        if (rightLrcIndex >= 0)
        {
            rightListView.positionViewAtIndex(rightLrcIndex, ListView.Center)
        }
    }

    // 背景视频
    VideoOutput
    {
        id: musicOutput
        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectCrop
    }

    MediaPlayer
    {
        id: videoPlayer
        videoOutput: musicOutput
        source: "file:///" + Dplay.videoPath
        autoPlay: true
        loops: MediaPlayer.Infinite
    }

    Rectangle
    {
        anchors.fill: parent
        color: "black"
        opacity: 0.45
    }

    Button
    {
        text: "← 返回"
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 20
        onClicked:
        {
            stack.pop()
        }
    }

    Row
    {
        anchors.centerIn: parent
        spacing: mainSpacing

        // 左耳播放器
        Column
        {
            width: playerColWidth
            spacing: colSpacing

            Text
            {
                text: "左耳"
                color: "white"
                font.pixelSize: titleFontSize
                anchors.horizontalCenter: parent.horizontalCenter
            }

            // 封面
            Rectangle
            {
                id: leftCoverBg
                width: coverSize
                height: coverSize
                radius: width / 2
                anchors.horizontalCenter: parent.horizontalCenter
                color: "white"

                // 阴影
                layer.enabled: true
                layer.effect: DropShadow
                {
                    radius: 15
                    samples: 20
                    color: "#80000000"
                }

                Image
                {
                    id: leftCover
                    anchors.fill: parent
                    anchors.margins: Math.max(4, 8)
                    source: "file:///" + Dplay.leftCover
                    fillMode: Image.PreserveAspectCrop
                    layer.enabled: true
                    layer.effect: OpacityMask
                    {
                        maskSource: Rectangle
                        {
                            width: leftCover.width
                            height: leftCover.height
                            radius: leftCover.width / 2
                            color: "white"
                        }
                    }

                    RotationAnimation on rotation
                    {
                        from: 0
                        to: 360
                        duration: 10000
                        loops: Animation.Infinite
                    }
                }
            }

            // 歌曲名
            Text
            {
                text: Dplay.leftMusicName
                color: "white"
                font.pixelSize: songFontSize
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Row
            {
                spacing: 10
                anchors.horizontalCenter: parent.horizontalCenter

                Text
                {
                    text: formatTime(Dplay.leftPosition)
                    color: "white"
                    font.pixelSize: timeFontSize
                    width: 40
                    horizontalAlignment: Text.AlignRight
                }

                Slider
                {
                    id: leftProgressSlider
                    width: parent.width * 2 / 3
                    from: 0
                    to: 1
                    value: Dplay.leftMusicDuration > 0 ? Dplay.leftPosition / Dplay.leftMusicDuration : 0

                    onMoved:
                    {
                        Dplay.setLeftPos(value * Dplay.leftMusicDuration)
                    }

                    Connections
                    {
                        target: Dplay
                        function onLeftPositionChanged()
                        {
                            if (!leftProgressSlider.pressed)
                            {
                                leftProgressSlider.value = Dplay.leftMusicDuration > 0 ? Dplay.leftPosition / Dplay.leftMusicDuration : 0
                            }
                        }
                    }

                    Component.onCompleted:
                    {
                        if (Dplay && Dplay.leftMusicDuration > 0)
                            leftProgressSlider.value = Dplay.leftPosition / Dplay.leftMusicDuration
                    }
                }

                Text
                {
                    text: formatTime(Dplay.leftMusicDuration)
                    color: "white"
                    font.pixelSize: timeFontSize
                    width: 40
                }
            }

            Row
            {
                spacing: 20
                anchors.horizontalCenter: parent.horizontalCenter

                Button 
                {
                    text: Dplay && Dplay.leftPlaying ? "⏸" : "▶"
                    onClicked: 
                    {
                        if (!Dplay) return
                        if(Dplay.leftPlaying)
                            Dplay.stopLeftMusic();
                        else if(!Dplay.leftPlaying)
                            Dplay.playLeft();
                    }
                }
            }
        }

        // 左音量滑块
        Slider
        {
            width: volSliderWidth
            height: parent.height * 3 / 4
            from: 0
            to: 1
            value: Dplay.leftVolume
            orientation: Qt.Vertical
            onMoved:
            {
                Dplay.setLeftVolume(value)
            }
        }

        // 左歌词
        ListView
        {
            id: leftListView
            width: lrcColWidth
            height: lrcHeight
            model: Dplay.leftLrc

            delegate: Text
            {
                width: parent.width
                text: modelData.text
                color: index === leftLrcIndex ? "purple" : "white"
                font.pixelSize: index === leftLrcIndex ? lrcHighlightFont : lrcNormalFont
                opacity: index === leftLrcIndex ? 1 : 0.5
                horizontalAlignment: Text.AlignHCenter
            }

            onCountChanged:
            {
                positionViewAtIndex(leftLrcIndex, ListView.Center)
            }
        }

        // 右歌词
        ListView
        {
            id: rightListView
            width: lrcColWidth
            height: lrcHeight
            model: Dplay.rightLrc

            delegate: Text
            {
                width: parent.width
                text: modelData.text
                color: index === rightLrcIndex ? "purple" : "white"
                font.pixelSize: index === rightLrcIndex ? lrcHighlightFont : lrcNormalFont
                opacity: index === rightLrcIndex ? 1 : 0.5
                horizontalAlignment: Text.AlignHCenter
            }

            onCountChanged:
            {
                positionViewAtIndex(rightLrcIndex, ListView.Center)
            }
        }

        // 右音量滑块
        Slider
        {
            width: volSliderWidth
            height: parent.height * 3 / 4
            from: 0
            to: 1
            value: Dplay.rightVolume
            orientation: Qt.Vertical
            onMoved:
            {
                Dplay.setRightVolume(value)
            }
        }

        // 右耳播放器
        Column
        {
            width: playerColWidth
            spacing: colSpacing

            Text
            {
                text: "右耳"
                color: "white"
                font.pixelSize: titleFontSize
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Rectangle
            {
                id: rightCoverBg
                width: coverSize
                height: coverSize
                radius: width / 2
                anchors.horizontalCenter: parent.horizontalCenter
                color: "white"

                layer.enabled: true
                layer.effect: DropShadow
                {
                    radius: 15
                    samples: 20
                    color: "#80000000"
                }

                Image
                {
                    id: rightCover
                    anchors.fill: parent
                    anchors.margins: Math.max(4, 8)
                    source: "file:///" + Dplay.rightCover
                    fillMode: Image.PreserveAspectCrop
                    layer.enabled: true
                    layer.effect: OpacityMask
                    {
                        maskSource: Rectangle
                        {
                            width: rightCover.width
                            height: rightCover.height
                            radius: rightCover.width / 2
                            color: "white"
                        }
                    }

                    RotationAnimation on rotation
                    {
                        from: 0
                        to: 360
                        duration: 10000
                        loops: Animation.Infinite
                    }
                }
            }

            // 歌曲名称
            Text
            {
                text: Dplay.rightMusicName
                color: "white"
                font.pixelSize: songFontSize
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Row
            {
                spacing: 10
                anchors.horizontalCenter: parent.horizontalCenter

                Text
                {
                    text: formatTime(Dplay.rightPosition)
                    color: "white"
                    font.pixelSize: timeFontSize
                    width: 40
                    horizontalAlignment: Text.AlignRight
                }

                Slider
                {
                    id: rightProgressSlider
                    width: parent.width * 2 / 3
                    from: 0
                    to: 1
                    value: Dplay.rightMusicDuration > 0 ? Dplay.rightPosition / Dplay.rightMusicDuration : 0

                    onMoved:
                    {
                        Dplay.setRightPos(value * Dplay.rightMusicDuration)
                    }

                    Connections
                    {
                        target: Dplay
                        function onRightPositionChanged()
                        {
                            if (!rightProgressSlider.pressed)
                            {
                                rightProgressSlider.value = Dplay.rightMusicDuration > 0 ? Dplay.rightPosition / Dplay.rightMusicDuration : 0
                            }
                        }
                    }

                    Component.onCompleted:
                    {
                        if (Dplay && Dplay.rightMusicDuration > 0)
                            rightProgressSlider.value = Dplay.rightPosition / Dplay.rightMusicDuration
                    }
                }

                Text
                {
                    text: formatTime(Dplay.rightMusicDuration)
                    color: "white"
                    font.pixelSize: timeFontSize
                    width: 40
                }
            }

            Row
            {
                anchors.horizontalCenter: parent.horizontalCenter

                Button 
                {
                    text: Dplay && Dplay.rightPlaying ? "⏸" : "▶"
                    onClicked: 
                    {
                        if (!Dplay) return
                        if(Dplay.rightPlaying)
                            Dplay.stopRightMusic();
                        else if(!Dplay.rightPlaying)
                            Dplay.playRight();
                    }
                }
            }
        }
    }

    Connections
    {
        target: Dplay
        function onLeftPositionChanged()
        {
            let pos = Dplay.leftPosition
            let list = Dplay.leftLrc
            for (let i = 0; i < list.length; i++)
            {
                if (i === list.length - 1 || (pos >= list[i].time && pos < list[i + 1].time))
                {
                    leftLrcIndex = i
                    break
                }
            }
        }

        function onRightPositionChanged()
        {
            let pos = Dplay.rightPosition
            let list = Dplay.rightLrc
            for (let i = 0; i < list.length; i++)
            {
                if (i === list.length - 1 || (pos >= list[i].time && pos < list[i + 1].time))
                {
                    rightLrcIndex = i
                    break
                }
            }
        }
    }
}