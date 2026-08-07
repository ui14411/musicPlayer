#include "HeaderFile/InitManage.h"

#include "HeaderFile/LoadlocalMusic.h"
#include "HeaderFile/LoadlocalVideo.h"
#include "HeaderFile/PlayMusic.h"
#include "HeaderFile/DoubleEar.h"
#include "HeaderFile/LoadImage.h"
#include "HeaderFile/AudioAnalyzerProxy.h"
#include "HeaderFile/PlayDouble.h"

#include <QCoreApplication>
#include <QFileInfo>

InitManage::InitManage(QObject* parent,
    PlayMusic* player,
    loadlocalMusic* musicload,
    DoubleEar* Dmusic,
    LoadbgImage* image,
    PlayDouble* doublePlayer,
    AudioAnalyzerProxy* analyzer,
    loadlocalVideo* videoload)
    : QObject(parent)
    , player(player)
    , musicload(musicload)
    , Dmusic(Dmusic)
    , image(image)
    , doublePlayer(doublePlayer)
    , analyzer(analyzer)
    , videoload(videoload)
{

}

void InitManage::start() {

    qDebug() << "start";
    m_progress = 10;
    emit progressChanged();
    QString videoPath = QCoreApplication::applicationDirPath() + "/video";
    QDir().mkpath(videoPath);
    videoload->loadVideoList(videoPath);
    
    m_progress = 30;
    emit progressChanged();
    player->setMusiclist();
    
    m_progress = 50;
    emit progressChanged();
    musicload->loadMusiclist();
    
    m_progress = 70;
    emit progressChanged();
    Dmusic->loadMusiclist();
    
    m_progress = 85;
    emit progressChanged();
    image->loadImagelist();
    
    m_progress = 95;
    emit progressChanged();

    connect(player, &PlayMusic::musicAnalyzerChanged, analyzer, &AudioAnalyzerProxy::loadMusic);
    connect(player, &PlayMusic::analyzerPositionChanged, analyzer, &AudioAnalyzerProxy::analyzePosition);
    connect(musicload, &loadlocalMusic::musicCoverChanged, this,[this](const QString& musicPath) {
        if (player->getCurrentMusicName() == QFileInfo(musicPath).completeBaseName()) {
            player->setCurrentMusicInfo();
        }
        });
    connect(musicload, &loadlocalMusic::musicLrcChanged, player, &PlayMusic::setMusicLrc);
    emit finished();
}