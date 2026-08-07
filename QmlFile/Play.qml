import QtQuick
import QtQuick.Controls
import QtMultimedia
import Qt5Compat.GraphicalEffects

Item
{
    id: root
    property int currentLrcIndex: -1
    property var bars: []
    objectName: "playPage"

    // 自适应基准尺寸：以较短边为基准
    property real baseUnit: Math.min(width, height)
    // 封面尺寸：占短边的 50%，上限 420，下限 220
    property real coverSize: Math.max(220, Math.min(420, baseUnit * 0.5))
    // 频谱半径
    property real spectrumRadius: coverSize * 0.48
    // 频谱最大高度
    property real spectrumMaxHeight: coverSize * 0.24

    anchors.fill: parent

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
                let t1 = mod[i].time
                let t2 = mod[i + 1].time
                if (pos >= t1 && pos < t2)
                {
                    idx = i
                    break
                }
            }
            currentLrcIndex = idx
        }
    }

    onCurrentLrcIndexChanged:
    {
        lrcView.positionViewAtIndex(currentLrcIndex, ListView.Center)
    }

    AudioOutput
    {
        id: videoAudio
        volume: 0
    }

    VideoOutput
    {
        id: musicOutput
        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectCrop
    }

    MediaPlayer
    {
        id: videoplayer
        videoOutput: musicOutput
        audioOutput:videoAudio
        source: player.videoPath
        autoPlay: true
        loops: MediaPlayer.Infinite
    }

    Rectangle
    {
        anchors.fill: parent
        color: "black"
        opacity: 0.4
    }

    // 返回按钮
    Button
    {
        id: backButton
        text: "← 返回"
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: 20
        anchors.topMargin: 20
        z: 999
        onClicked: stack.pop()
    }

    // 主布局
    Row
    {
        anchors.centerIn: parent
        // 间距：占宽度的 9%，下限 50，上限 200
        spacing: Math.max(50, Math.min(200, parent.width * 0.09))

        // ===== 左侧：封面 + 信息 + 歌词偏移滑块 =====
        Row
        {
            spacing: Math.max(10, root.width * 0.02)
            anchors.verticalCenter: parent.verticalCenter

            Column
            {
                spacing: Math.max(10, baseUnit * 0.02)
                anchors.verticalCenter: parent.verticalCenter

                Item
                {
                    id: coverArea
                    width: coverSize
                    height: coverSize

                    // 专辑封面
                    Rectangle
                    {
                        id: coverContainer
                        anchors.centerIn: parent
                        width: coverSize * 0.88
                        height: width
                        radius: width / 2
                        clip: false

                        Image
                        {
                            id: coverImage
                            anchors.fill: parent
                            source: "file:///" + player.currentMusicCover
                            fillMode: Image.PreserveAspectCrop
                            layer.enabled: true
                            layer.effect: OpacityMask
                            {
                                maskSource: Rectangle
                                {
                                    width: coverImage.width
                                    height: coverImage.height
                                    radius: coverImage.width / 2
                                    color: "white"
                                }
                            }

                            RotationAnimation on rotation
                            {
                                from: 0
                                to: 360
                                duration: 10000
                                loops: Animation.Infinite
                                running: player.isPlaying
                            }
                        }
                    }

                    // 圆形频谱
                    Canvas
                    {
                        id: spectrumCanvas
                        anchors.fill: parent
                        z: 10
                        onPaint:
                        {
                            let ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)
                            if (bars.length === 0)
                                return
                            let cx = width / 2
                            let cy = height / 2
                            let radius = spectrumRadius
                            let count = bars.length
                            let maxHeight = spectrumMaxHeight
                            for (let i = 0; i < count; i++)
                            {
                                let angle = (i / count) * Math.PI * 2
                                let value = Math.min(bars[i] * 50, maxHeight)
                                let r1 = radius
                                let r2 = radius + value
                                let x1 = cx + Math.cos(angle) * r1
                                let y1 = cy + Math.sin(angle) * r1
                                let x2 = cx + Math.cos(angle) * r2
                                let y2 = cy + Math.sin(angle) * r2
                                ctx.beginPath()
                                ctx.moveTo(x1, y1)
                                ctx.lineTo(x2, y2)
                                ctx.strokeStyle = player.spectrumColor
                                ctx.lineWidth = Math.max(2, coverSize * 0.008)
                                ctx.stroke()
                            }
                        }
                    }

                    Connections
                    {
                        target: audioAnalyzer
                        function onSpectrumChanged(data)
                        {
                            bars = data
                            spectrumCanvas.requestPaint()
                        }
                    }
                }

                // 歌名
                Text
                {
                    text: player.currentMusicName
                    color: "black"
                    font.pixelSize: Math.max(18, Math.min(28, baseUnit * 0.038))
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: coverSize
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                }

                // 歌手
                Text
                {
                    text: player.currentMusicSinger
                    color: "black"
                    font.pixelSize: Math.max(14, Math.min(20, baseUnit * 0.024))
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: coverSize
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                }

                // 音量滑块
                Slider
                {
                    width: coverSize
                    from: 0
                    to: 1
                    value: player.volume
                    onMoved:player.volume = value
                }
            }

            // 垂直歌词偏移滑块
            Slider
            {
                id: lrcOffsetSlider
                width: 30
                height: Math.max(200, Math.min(400, baseUnit * 0.55))
                from: -5000
                to: 5000
                stepSize: 100
                value: player.lyricOffset
                orientation: Qt.Vertical
                anchors.verticalCenter: parent.verticalCenter
                onMoved: player.lyricOffset = value
            }
        }

        // ===== 右侧：歌词 =====
        ListView
        {
            id: lrcView
            // 宽度：占窗口 50%，下限 380，上限 700
            width: Math.max(380, Math.min(700, root.width * 0.5))
            // 高度：占窗口 82%，下限 350，上限 620
            height: Math.max(350, Math.min(620, root.height * 0.82))
            highlightMoveDuration: 200
            model: player.musicLrc
            clip: true

            delegate: Text
            {
                text: modelData.text
                color: index === currentLrcIndex ? player.lrcColor : "white"
                font.bold: index === currentLrcIndex
                scale: index === currentLrcIndex ? 1.15 : 1.0
                font.pixelSize: index === currentLrcIndex
                    ? Math.max(18, Math.min(28, baseUnit * 0.035))
                    : Math.max(15, Math.min(22, baseUnit * 0.028))
                opacity: index === currentLrcIndex ? 1.0 : 0.6
                width: lrcView.width
                horizontalAlignment: Text.AlignHCenter

                Behavior on font.pixelSize
                {
                    NumberAnimation { duration: 150 }
                }
                Behavior on color
                {
                    ColorAnimation { duration: 150 }
                }
            }
        }
    }
}