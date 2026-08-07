#pragma once

#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QDir>
#include <QFileInfo>
#include <QVector>

class PlayDouble : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString leftMusicName
        READ getcurrentLeftMusicPath
        NOTIFY leftMusicNameChanged)

    Q_PROPERTY(QString rightMusicName
        READ getcurrentRightMusicPath
        NOTIFY rightMusicNameChanged)

    Q_PROPERTY(float leftVolume
        READ getLeftVolume WRITE setLeftVolume
        NOTIFY leftVolumeChanged)

    Q_PROPERTY(float rightVolume
        READ getRightVolume WRITE setRightVolume
        NOTIFY rightVolumeChanged)

    Q_PROPERTY(QString leftCover
        READ getleftCover
        NOTIFY leftCoverChanged)

    Q_PROPERTY(QString rightCover
        READ getrightCover
        NOTIFY rightCoverChanged)

    Q_PROPERTY(QVariantList leftLrc
        READ getleftLrc
        NOTIFY leftLrcChanged)

    Q_PROPERTY(QVariantList rightLrc
        READ getRightLrc
        NOTIFY rightLrcChanged)

    Q_PROPERTY(qint64 leftPosition
        READ getleftPos
        NOTIFY leftPosChanged)

    Q_PROPERTY(qint64 rightPosition
        READ getrightPos
        NOTIFY rightPosChanged)

    Q_PROPERTY(qint64 leftMusicDuration
        READ getleftMusicDuration
        NOTIFY leftTotaltimeChanged)

    Q_PROPERTY(qint64 rightMusicDuration
        READ getrightMusicDuration
        NOTIFY rightTotaltimeChanged)

    Q_PROPERTY(bool leftPlaying
        READ getleftPlaying
        NOTIFY leftMusicStatusChanged)

    Q_PROPERTY(bool rightPlaying
        READ getrightPlaying
        NOTIFY rightMusicStatusChanged)

    Q_PROPERTY(QString videoPath
        READ getVideoPath
        NOTIFY videoPathChanged)

public:
    
    PlayDouble(QObject* parent = nullptr);

    struct lrcLine
    {
        qint64 time;
        QString text;
    };

public:

   Q_INVOKABLE void loadDoubleMusic(const QString& path);

   Q_INVOKABLE void setLeftVolume(const float& volume);
   Q_INVOKABLE void setRightVolume(const float& volume);

   Q_INVOKABLE void setLeftPos(const int& pos);
   Q_INVOKABLE void setRightPos(const int& pos);

   Q_INVOKABLE void playLeftMusic();
   Q_INVOKABLE void playRightMusic();

   Q_INVOKABLE void playLeft() { leftPlayer->play(); };
   Q_INVOKABLE void playRight() { rightPlayer->play(); };

   Q_INVOKABLE void stopLeftMusic();
   Q_INVOKABLE void stopRightMusic();

public:

    QString getcurrentLeftMusicPath() const { return currentLeftMusicName; };
    QString getcurrentRightMusicPath() const { return currentRightMusicName; };

    QString getleftCover() const { return currentLeftMusicCover; };
    QString getrightCover() const { return currentRightMusicCover; };

    float getLeftVolume() const { return m_leftVolume; };
    float getRightVolume() const { return m_rightVolume; };

    QVariantList getleftLrc() const;
    QVariantList getRightLrc() const;

    qint64 getleftPos() const { return m_leftPosition; };
    qint64 getrightPos() const { return m_rightPosition; };

    qint64 getleftMusicDuration() const { return leftPlayer->duration(); };
    qint64 getrightMusicDuration() const { return rightPlayer->duration(); };

    bool getleftPlaying() const { return leftPlayer && leftPlayer->playbackState() == QMediaPlayer::PlayingState;; };
    bool getrightPlaying() const { return rightPlayer && rightPlayer->playbackState() == QMediaPlayer::PlayingState;; };

    QString getVideoPath() const { return m_videoPath; };

signals:
    void leftMusicPathChanged();
    void rightMusicPathChanged();

    void leftMusicNameChanged();
    void rightMusicNameChanged();

    void leftVolumeChanged();
    void rightVolumeChanged();

    void leftCoverChanged();
    void rightCoverChanged();

    void leftLrcChanged();
    void rightLrcChanged();

    void leftPosChanged();
    void rightPosChanged();

    void leftTotaltimeChanged();
    void rightTotaltimeChanged();

    void leftMusicStatusChanged();
    void rightMusicStatusChanged();
 
    void videoPathChanged();

private:
    void setleftMusicLrc(const QString& musicName);
    void setrightMusicLrc(const QString& musicName);

private:

    QString currentLeftMusicPath;
    QString currentRightMusicPath;

    QString currentLeftMusicName;
    QString currentRightMusicName;

    QString currentLeftMusicCover;
    QString currentRightMusicCover;

    QMediaPlayer* leftPlayer;
    QMediaPlayer* rightPlayer;

    QAudioOutput* leftAudioOutput;
    QAudioOutput* rightAudioOutput;

    float m_leftVolume = 1.0f;
    float m_rightVolume = 1.0f;

    QVector<lrcLine> m_leftLrc;
    QVector<lrcLine> m_rightLrc;

    qint64 m_leftPosition = 0;
    qint64 m_rightPosition = 0;

    qint64 m_leftMusicTotal = 0;
    qint64 m_rightMusicTotal = 0;

    bool m_isleftPlaying;
    bool m_isrightPlaying;

    QString m_videoPath;
};