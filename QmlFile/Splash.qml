import QtQuick
import QtQuick.Controls

Rectangle {
    id: splashRoot
    
    // ✅ 接收 main.qml 传入的 StackView
    property var stack: null
    
    property int progress: 0
    property bool finished: false
    
    anchors.fill: parent
    color: "#1a2332"
    
    Connections {
        target: initManage
        
        function onProgressChanged() {
            console.log("[Splash] Progress:", initManage.progress)
            splashRoot.progress = initManage.progress
            progressBar.width = (splashRoot.progress / 100) * progressContainer.width
            updateStatusText()
        }
        
        function onFinished() {
            console.log("[Splash] ✅ Finished!")
            splashRoot.finished = true
            fadeOutAnimation.start()
        }
    }
    
    // ==== 背景网格 ====
    Grid {
        anchors.fill: parent
        columns: 20
        rows: 15
        opacity: 0.04
        
        Repeater {
            model: 300
            Rectangle {
                width: parent.width / 20
                height: parent.height / 15
                color: "#7a8ba8"
                opacity: Math.random() > 0.7 ? 0.2 : 0.03
            }
        }
    }
    
    // ==== 扫描线效果 ====
    Rectangle {
        anchors.fill: parent
        color: "transparent"
        
        Rectangle {
            width: parent.width
            height: 2
            color: "#7a8ba8"
            opacity: 0.08
            x: 0
            y: (parent.height / 2) + Math.sin(Date.now() / 2000) * 100
            
            NumberAnimation on y {
                from: -50
                to: parent.height + 50
                duration: 4000
                loops: Animation.Infinite
            }
        }
    }
    
    // ==== 主标题 "Loading..." ====
    Text {
        id: loadingTitle
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -60
        
        text: "Loading..."
        color: "#9bb7d4"
        font.pixelSize: 42
        font.bold: true
        font.family: "Courier New"
        
        Text {
            anchors.left: loadingTitle.right
            anchors.bottom: loadingTitle.bottom
            anchors.bottomMargin: 2
            
            text: "|"
            color: "#9bb7d4"
            font.pixelSize: 42
            font.bold: true
            
            SequentialAnimation on opacity {
                running: true
                loops: Animation.Infinite
                NumberAnimation { from: 0; to: 1; duration: 500 }
                NumberAnimation { from: 1; to: 0; duration: 500 }
            }
        }
    }
    
    // ==== 进度条容器 ====
    Rectangle {
        id: progressContainer
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: loadingTitle.bottom
        anchors.topMargin: 40
        
        width: 320
        height: 3
        radius: 1.5
        color: "#2a3a4e"
        
        Rectangle {
            anchors.fill: parent
            color: "transparent"
            border.color: "#6a8aaa"
            border.width: 0.5
            opacity: 0.3
            radius: parent.radius
        }
        
        Rectangle {
            id: progressBar
            width: 0
            height: parent.height
            radius: parent.radius
            color: "#7ab7d4"
            
            Rectangle {
                anchors.right: parent.right
                width: 30
                height: parent.height
                color: "#7ab7d4"
                opacity: 0.3
                radius: parent.radius
            }
            
            Behavior on width {
                NumberAnimation { duration: 200 }
            }
        }
    }
    
    // ==== 进度百分比 ====
    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: progressContainer.bottom
        anchors.topMargin: 12
        
        text: Math.round(progress) + "%"
        color: "#6a8aaa"
        font.pixelSize: 13
        font.family: "Courier New"
    }
    
    // ==== 状态文字 ====
    Text {
        id: statusText
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.verticalCenter
        anchors.topMargin: 120
        
        text: "正在初始化..."
        color: "#8aaac0"
        font.pixelSize: 14
        font.family: "Courier New"
        
        Behavior on text {
            SequentialAnimation {
                NumberAnimation { target: statusText; property: "opacity"; from: 1; to: 0.3; duration: 150 }
                PropertyAction { }
                NumberAnimation { target: statusText; property: "opacity"; from: 0.3; to: 1; duration: 150 }
            }
        }
    }
    
    // ==== 底部版本信息 ====
    Text {
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: 40
        
        text: "v2.0.1  |  SYSTEM INITIALIZING"
        color: "#4a5a6e"
        font.pixelSize: 11
        font.family: "Courier New"
        font.letterSpacing: 2
    }
    
    // ==== 更新状态文字 ====
    function updateStatusText() {
        if (progress < 10) {
            statusText.text = "正在初始化系统..."
        } else if (progress < 30) {
            statusText.text = "正在加载音乐列表..."
        } else if (progress < 50) {
            statusText.text = "正在加载本地音乐..."
        } else if (progress < 70) {
            statusText.text = "正在加载双耳音乐..."
        } else if (progress < 85) {
            statusText.text = "正在加载图片列表..."
        } else if (progress < 100) {
            statusText.text = "正在连接信号..."
        } else {
            statusText.text = "加载完成 ✓"
        }
    }
    
    // ==== 淡出动画 ====
    NumberAnimation {
        id: fadeOutAnimation
        target: splashRoot
        property: "opacity"
        from: 1
        to: 0
        duration: 500
        easing.type: Easing.InOutCubic
        onFinished: {
            // ✅ 使用传入的 stack 跳转到 Home
            if (stack) {
                console.log("[Splash] ✅ Pushing Home.qml via stack")
                stack.replace("Home.qml")
            } else {
                console.log("[Splash] ❌ stack is null!")
            }
        }
    }
}