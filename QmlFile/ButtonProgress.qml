import QtQuick
import QtQuick.Controls

Rectangle 
{
    id: root
    height: 90
    color: "#202020"
    opacity:0.4
    z: 100
    visible:player.panelModel === 0

    property var stack
    property bool flag: false

    Component.onCompleted: 
    {
        stack = StackView.view
    }

    MouseArea {
        anchors.fill: parent
        z: 1
        onClicked: {
            if (stack.currentItem && stack.currentItem.objectName === "playPage") {
                stack.pop()
                player.stopPreview()
            } else {
                stack.push("Play.qml")
                player.playVideo(player.currentMusicName)
            }
        }
    }

    // 辅助函数：格式化时间（毫秒 -> mm:ss）
    function formatTime(ms) 
    {
        if (ms <= 0) return "00:00"
        var seconds = Math.floor(ms / 1000)
        var minutes = Math.floor(seconds / 60)
        seconds = seconds % 60
        return (minutes < 10 ? "0" + minutes : minutes) + ":" +
               (seconds < 10 ? "0" + seconds : seconds)
    }

    // 左侧：歌曲封面和名称
    Row 
    {
        id:musicInfo
        anchors.left: parent.left
        anchors.leftMargin: 15
        anchors.verticalCenter: parent.verticalCenter
        spacing: 12
        // 封面占位（如有专辑封面可替换）
        Rectangle 
        {
            width: 60
            height: 60
            radius: 8
            Image
            {
                anchors.fill: parent
                source: "file:///" + player.currentMusicCover
                fillMode: Image.PreserveAspectCrop
            }
        }

        Column 
        {
            spacing: 4
            Text 
            {
                text: player ? player.currentMusicName : ""
                color: "white"
                font.pixelSize: 16
                font.bold: true
            }
            Text 
            {
                text: player ? player.currentMusicSinger : ""
                color: "#bbbbbb"
                font.pixelSize: 12
            }
        }
    }

    // 中央：控制按钮和进度条
    Column 
    {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8
        z:2
        // 控制按钮
        Row 
        {
            spacing: 20
            anchors.horizontalCenter: parent.horizontalCenter
            
            Button 
            {
                text: "⏮"
                onClicked: if (player) player.prevMusic()
            }
            Button 
            {
                text: player && player.playing ? "⏸" : "▶"
                onClicked: 
                {
                    if (!player) return
                    if(player.playing)
                        player.Pause();
                    else if(!player.playing)
                       player.Play();
                }
            }
            Button 
            {
                text: "⏭"
                onClicked: if (player) player.nextMusic()
            }
            Button 
            {
                text: 
                {
                    if (player.playmodel === 2) return "顺序播放"
                    else if (player.playmodel === 1) return "随机播放" 
                    else if (player.playmodel === 0) return "单曲循环"    
                } 
                onClicked: player.switchModel()
            }
            Button
            {
                id: modeButton

                text:
                {
                    if(player.playpattern === 0) return "原声 ▼"
                    else if(player.playpattern === 1) return "人声 ▼"
                    else if(player.playpattern === 2) return "伴奏 ▼"
                    else if(player.playpattern === 3) return "环绕 ▼"
                    return "模式 ▼"
                }

                onClicked:
                {
                    if(modePopup.opened)
                        modePopup.close()
                    else
                        modePopup.open()
                }
            }
        }
        // 进度条区域
        Row 
        {
            spacing: 10
            anchors.horizontalCenter: parent.horizontalCenter
            Text 
            {
                id: currentTimeLabel
                text: formatTime(player ? player.position : 0)
                color: "white"
                font.pixelSize: 12
                width: 40
                horizontalAlignment: Text.AlignRight
            }

            Slider 
            {
                id: progressSlider
                width: 400
                from: 0
                to: 1
                value: 0

                onMoved: 
                {
                    if (player)
                        player.setPosition(value * player.musicDuration)
                }
            }

            Text 
            {
                text: formatTime(player ? player.musicDuration : 0)
                color: "white"
                font.pixelSize: 12
                width: 40
            }
        }

        Connections 
        {
            target: player

            onMusicPositionChanged:
            {
                if (!progressSlider.pressed && player && player.musicDuration > 0) 
                {
                    progressSlider.value = player.musicPosition / player.musicDuration
                }
            }
        }

        Component.onCompleted: 
        {
            if (player && player.musicDuration > 0)
                progressSlider.value = player.musicPosition / player.musicDuration
        }
    }

    Popup
    {
        id: modePopup

        x: modeButton.mapToItem(parent, 0, 0).x + modeButton.width / 2 - width / 2
        y: modeButton.mapToItem(parent,0,0).y - height - 8

        width: 140
        padding: 0
        height: 160

        modal: false
        focus: true

        closePolicy:
            Popup.CloseOnEscape |
            Popup.CloseOnPressOutside

        enter: Transition
        {
            NumberAnimation
            {
                property: "opacity"
                from: 0
                to: 1
                duration: 150
            }
        }

        exit: Transition
        {
            NumberAnimation
            {
                property: "opacity"
                from: 1
                to: 0
                duration: 120
            }
        }

        background: Rectangle
        {
            radius: 8
            color: "#2E2E2E"
            border.color: "#505050"
            border.width: 1
        }

        Column
        {
            width: parent.width
            spacing: 0

            Repeater
            {
                model:
                [
                    "原声",
                    "人声",
                    "伴奏",
                    "环绕"
                ]

                delegate: Rectangle
                {
                    width: parent.width
                    height: 40

                    color:
                    {
                        if(mouse.containsMouse)
                            return "#505050"

                        if(index === player.playpattern)
                            return "#3A7AFE"

                        return "transparent"
                    }

                    Text
                    {
                        anchors.centerIn: parent

                        text:
                        {
                            if(index === player.playpattern)
                                return "✓  " + modelData

                            return modelData
                        }

                        color: "white"
                        font.pixelSize: 15
                    }

                    MouseArea
                    {
                        id: mouse

                        anchors.fill: parent
                        hoverEnabled: true

                        onClicked:
                        {
                            player.playpattern = index
                            modePopup.close()
                        }
                    }
                }
            }
        }
     }  
}