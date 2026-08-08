import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Dialogs

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
        height: item ? item.height : 90
        source: "ButtonProgress.qml"
        onLoaded: {
            loader.item.stack = stack
        }
    }

    Connections
    {
        target: player

        function onPatternFileMissing()
        {
            missingFileDialog.open()
        }
    }

    MessageDialog
    {
        id: missingFileDialog

        title: "提示"
        text: "未找到文件，请右键歌曲列表对应歌曲检查文件完整性。"
    }
}
