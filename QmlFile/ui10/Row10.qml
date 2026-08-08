import QtQuick
import Qt5Compat.GraphicalEffects
import nusicvideoqml

// 列表行：音乐 / 双耳（两张封面）/ 图片，三态由属性控制，行为交给外层信号
Item
{
    id: row

    property int idx: 0
    property string title: ""
    property string sub: ""
    property string meta: ""
    property string cover: ""
    property string cover2: ""
    property bool twoCovers: false
    property real rowH: 52
    property real cvPx: 36
    property int cvR: 8
    property bool selected: false
    property bool hovered: false
    property bool hairline: false
    property bool gridCard: false
    property string fam: Theme.family
    property string serifFam: Theme.serif
    property real titlePx: 13.5
    property real subPx: 11.5
    property bool idxSerif: false
    property bool squareCover: false

    signal enter()
    signal exit()
    signal clickedLeft()
    signal clickedRight()

    width: parent ? parent.width : 0
    height: gridCard ? cvPx + 62 : rowH

    Rectangle
    {
        anchors.fill: parent
        radius: row.hairline ? 0 : Theme.r1
        color: row.hovered ? Theme.hov : (row.selected ? Theme.sel : "transparent")
        border.color: row.gridCard ? Theme.brd : "transparent"
        border.width: row.gridCard ? 1 : 0

        Behavior on color
        {
            ColorAnimation { duration: 130 }
        }
    }

    // 序号
    Text
    {
        id: idxTxt
        visible: !row.gridCard && !row.twoCovers && row.meta !== ""
        width: 34
        height: parent.height
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: row.idx
        color: row.selected ? Theme.acc : Theme.tx3
        font.pixelSize: row.idxSerif ? 20 : 12
        font.family: row.idxSerif ? row.serifFam : Theme.mono
    }

    // 封面（双耳是两张叠在一起）
    Item
    {
        id: cvBox
        x: row.gridCard ? 10 : (idxTxt.visible ? idxTxt.width + 8 : 12)
        anchors.verticalCenter: row.gridCard ? undefined : parent.verticalCenter
        y: row.gridCard ? 10 : 0
        width: row.twoCovers ? row.cvPx + 14 : row.cvPx
        height: row.cvPx

        Rectangle
        {
            id: cvA
            width: row.cvPx
            height: row.cvPx
            radius: row.squareCover ? Theme.r1 : (row.twoCovers ? width / 2 : row.cvR)
            color: Theme.pan2
            border.color: Theme.pan
            border.width: row.twoCovers ? 2 : 0
            anchors.right: row.twoCovers ? parent.right : undefined
            anchors.verticalCenter: row.twoCovers ? parent.verticalCenter : undefined

            Image
            {
                anchors.fill: parent
                anchors.margins: 1
                source: row.cover
                fillMode: Image.PreserveAspectCrop
                smooth: true
                layer.enabled: true
                layer.effect: OpacityMask
                {
                    maskSource: Rectangle
                    {
                        width: cvA.width - 2
                        height: cvA.height - 2
                        radius: row.squareCover ? Theme.r1 : (row.twoCovers ? width / 2 : row.cvR)
                        color: "white"
                    }
                }
            }
        }

        Rectangle
        {
            id: cvB
            visible: row.twoCovers
            width: row.cvPx
            height: row.cvPx
            radius: width / 2
            color: Theme.pan2
            border.color: Theme.pan
            border.width: 2
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter

            Image
            {
                anchors.fill: parent
                anchors.margins: 1
                source: row.cover2
                fillMode: Image.PreserveAspectCrop
                smooth: true
                layer.enabled: true
                layer.effect: OpacityMask
                {
                    maskSource: Rectangle
                    {
                        width: cvB.width - 2
                        height: cvB.height - 2
                        radius: width / 2
                        color: "white"
                    }
                }
            }
        }
    }

    // 文字
    Column
    {
        id: txtCol
        visible: !row.gridCard
        x: cvBox.x + cvBox.width + 12
        anchors.verticalCenter: parent.verticalCenter
        spacing: 3
        width: row.width - x - 90

        Text
        {
            width: parent.width
            text: row.title
            color: row.selected ? Theme.acc : Theme.tx
            font.pixelSize: row.titlePx
            font.bold: true
            font.family: row.fam
            elide: Text.ElideRight
        }

        Text
        {
            visible: row.sub !== ""
            width: parent.width
            text: row.sub
            color: Theme.tx3
            font.pixelSize: row.subPx
            font.family: row.fam
            elide: Text.ElideRight
        }
    }

    // 网格卡的文字放在封面下面
    Column
    {
        visible: row.gridCard
        x: 12
        anchors.left: parent.left
        anchors.top: cvBox.bottom
        anchors.topMargin: 8
        width: row.width - 24
        spacing: 2

        Text
        {
            width: parent.width
            text: row.title
            color: row.selected ? Theme.acc : Theme.tx
            font.pixelSize: 13
            font.bold: true
            font.family: row.fam
            elide: Text.ElideRight
        }

        Text
        {
            visible: row.sub !== ""
            width: parent.width
            text: row.sub
            color: Theme.tx3
            font.pixelSize: 11
            font.family: row.fam
            elide: Text.ElideRight
        }
    }

    // 右侧信息（时长 / 张数）
    Text
    {
        visible: !row.gridCard && row.meta !== ""
        anchors.right: parent.right
        anchors.rightMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        text: row.meta
        color: row.twoCovers ? Theme.acc : Theme.tx3
        font.pixelSize: 12
        font.family: Theme.mono
    }

    // 网格卡的时长角标
    Rectangle
    {
        visible: row.gridCard && row.meta !== ""
        anchors.right: parent.right
        anchors.rightMargin: 18
        anchors.top: parent.top
        anchors.topMargin: 18
        width: mt.implicitWidth + 12
        height: 18
        radius: 6
        color: "#80000000"

        Text
        {
            id: mt
            anchors.centerIn: parent
            text: row.meta
            color: "white"
            font.pixelSize: 10
            font.family: Theme.mono
        }
    }

    // 细分隔线（h6 / h7）
    Rectangle
    {
        visible: row.hairline
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: Theme.brd
    }

    MouseArea
    {
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        cursorShape: Qt.PointingHandCursor
        onEntered:
        {
            row.hovered = true
            row.enter()
        }
        onExited:
        {
            row.hovered = false
            row.exit()
        }
        onClicked: function (mouse)
        {
            if (mouse.button === Qt.RightButton)
                row.clickedRight()
            else
                row.clickedLeft()
        }
    }
}
