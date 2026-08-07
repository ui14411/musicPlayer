import QtQuick
import QtQuick.Window
import QtQuick.Controls

Window 
{
    visible: true
    width: 1280
    height: 720
    color: "#1a2332"  // 与 Splash 背景一致
    title: "Music Player"
    
    StackView 
    {
        id: stack
        anchors.fill: parent

        initialItem: Splash
        {
            stack: stack
        }
    }
    Loader 
    {
        id: loader
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 90
        source: "ButtonProgress.qml"
        onLoaded: {
            loader.item.stack = stack
        }
    }
}