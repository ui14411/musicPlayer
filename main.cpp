#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext.h>
#include <Windows.h>
#include <QSGRendererInterface>
#include <QQuickWindow>
#include <QMediaDevices>
#include <QDebug>
#include <iostream>
#include <onnxruntime_cxx_api.h>
#include <QThread>
#include <QFile>
#include <cfloat>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <QTimer>
#include <QDir>

#include "kiss_fft.h"
#include "HeaderFile/LoadlocalMusic.h"
#include "HeaderFile/LoadlocalVideo.h"
#include "HeaderFile/PlayMusic.h"
#include "HeaderFile/AudioSeparator.h"
#include "HeaderFile/DoubleEar.h"
#include "HeaderFile/LoadImage.h"
#include "HeaderFile/AudioAnalyzerProxy.h"
#include "HeaderFile/PlayDouble.h"
#include "HeaderFile/InitManage.h"

int main(int argc, char* argv[])
{
#if defined(Q_OS_WIN) && QT_VERSION_CHECK(5, 6, 0) <= QT_VERSION && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    loadlocalVideo videoload;

    QString musicPath = QCoreApplication::applicationDirPath() + "/music";
    QString videoPath = QCoreApplication::applicationDirPath() + "/video";
    QString doubleMusicPath = QCoreApplication::applicationDirPath() + "/[double]music";
    QString bgPath = QCoreApplication::applicationDirPath() + "/Image";

    QDir().mkpath(musicPath);
    QDir().mkpath(videoPath);
    QDir().mkpath(doubleMusicPath);
    QDir().mkpath(bgPath);

    PlayMusic player(nullptr,musicPath);
    loadlocalMusic musicload(nullptr,musicPath);
    DoubleEar Dmusic(nullptr, doubleMusicPath);
    LoadbgImage image(nullptr, bgPath);
    PlayDouble doublePlayer(nullptr);
    AudioAnalyzerProxy analyzer;
    InitManage init(nullptr, &player, &musicload, &Dmusic, &image, &doublePlayer, &analyzer, &videoload);

    engine.rootContext()->setContextProperty("music", &musicload);
    engine.rootContext()->setContextProperty("video", &videoload);
    engine.rootContext()->setContextProperty("player", &player);
    engine.rootContext()->setContextProperty("Dmusic", &Dmusic);
    engine.rootContext()->setContextProperty("image", &image);
    engine.rootContext()->setContextProperty("audioAnalyzer", &analyzer);
    engine.rootContext()->setContextProperty("Dplay", &doublePlayer);
    engine.rootContext()->setContextProperty("initManage", &init);

    engine.loadFromModule("nusicVideoQml","main");

    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/nusicvideoqml/QmlFile/main.qml")));

    if (engine.rootObjects().isEmpty())
        return -1;

    QTimer::singleShot(200, &init, &InitManage::start);

    return app.exec();
}
