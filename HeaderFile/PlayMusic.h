#pragma once
#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVector>
#include <QThread>
#include <QSettings>
#include <QTimer>
#include <QCoreApplication>

#include "HeaderFile/AudioSeparator.h"

struct lrcLine
{
	qint64 time;
	QString text;
};

class PlayMusic :public QObject
{
	Q_OBJECT

		Q_PROPERTY(QString currentMusicName
			READ getCurrentMusicName
			NOTIFY musicNameChanged)

		Q_PROPERTY(QString currentMusicSinger
			READ getCurrentMusicSinger
			NOTIFY musicSingerChanged)

		Q_PROPERTY(float volume
			READ volumce
			WRITE setVolume
			NOTIFY volumeChanged)

		Q_PROPERTY(QUrl bgPath
			READ changedBg
			NOTIFY bgchanged)

		Q_PROPERTY(QVariantList musicLrc
			READ getmusicLrc
			NOTIFY musicChanged)

		Q_PROPERTY(qint64 position
			READ lrcPosition
			NOTIFY musicLrcChanged)

		Q_PROPERTY(qint64 musicPosition
			READ getmusicPosition
			NOTIFY musicPositionChanged)

		Q_PROPERTY(qint64 musicDuration
			READ getmusicTotalTime
			NOTIFY musicDurationChanged)

		Q_PROPERTY(bool playing
			READ isPlaying
			NOTIFY musicStatusChanged)

		Q_PROPERTY(playStatus playmodel
			READ getPlayModel WRITE setPlayModel
			NOTIFY playStatusChanged)

		Q_PROPERTY(QUrl videoPath
			READ getVideoPath
			NOTIFY videoPathChanged)

		Q_PROPERTY(int playpattern
			READ getPlayPattern WRITE setPlayPattern
			NOTIFY playModelsChanged)

		Q_PROPERTY(QUrl previewPath
			READ getPreviewPath
			NOTIFY previewpathChange)

		Q_PROPERTY(QString currentMusicCover
			READ getCurrentMusicCover
			NOTIFY musicCoverChanged)

		Q_PROPERTY(QString lrcColor
			READ getLrcColor WRITE setLrcColor
			NOTIFY lrcColorChanged)

		Q_PROPERTY(QString spectrumColor
			READ getSpecColor WRITE setSpectrumColor
			NOTIFY spectrumColorChanged)

		Q_PROPERTY(qint64 lyricOffset
			READ getlyricOffset WRITE setlyricOffset
			NOTIFY lyricOffsetChanged)

		Q_PROPERTY(float transprant
			READ getTransprant WRITE saveTransparent
			NOTIFY transprantChanged)

		Q_PROPERTY(int panelModel
			READ getPanelmodel WRITE setPanelmodel
			NOTIFY panelModelChanged)
public:
	enum playStatus
	{
		RepeatOne,
		Random,
		Sequential
	};
	Q_ENUM(playStatus);

	enum playModels
	{
		Normal,
		PureVoice,
		Accompaniment,
		Surrounding

	};
	Q_ENUM(playModels);
public:
	PlayMusic(QObject* parent = nullptr,QString _filePath = "");
	~PlayMusic();
public:
	Q_INVOKABLE void playMusic(const QString& musicPath);
	Q_INVOKABLE void playVideo(const QString& videoPath);
	Q_INVOKABLE void setVolume(const float& value);
	Q_INVOKABLE float volumce() const;
	Q_INVOKABLE void setMusiclist();
	Q_INVOKABLE void setNormMusiclist(const QVariantList& paths);
	Q_INVOKABLE void setbgImage(const QUrl& bgPath);
	Q_INVOKABLE void addbgImage(const QUrl& bgPath);
	Q_INVOKABLE void setbgImage();
	Q_INVOKABLE void setPosition(const qint64& pos);
	Q_INVOKABLE void Pause();
	Q_INVOKABLE void Play();
	Q_INVOKABLE void nextMusic();
	Q_INVOKABLE void prevMusic();
	Q_INVOKABLE void setPlayModel(playStatus model);
	Q_INVOKABLE void switchModel();
	Q_INVOKABLE void setMusicModel();
	Q_INVOKABLE void setPlayPattern(int pattern);
	Q_INVOKABLE void switchPattern();
	Q_INVOKABLE void playNormMusic();
	Q_INVOKABLE void playPureVoiceMusic();
	Q_INVOKABLE void playAccompanimentMusic();
	Q_INVOKABLE void playSurroundingMusic();
	Q_INVOKABLE void playPreviewMusic(const QString& videoPath);
	Q_INVOKABLE void playPreviewImage(const QString& imagePath);
	Q_INVOKABLE void stopMusic();
	Q_INVOKABLE  QVariantList getAllPaths() const;
	Q_INVOKABLE void setLrcColor(const QString& color);
	Q_INVOKABLE void setSpectrumColor(const QString& color);
	Q_INVOKABLE void setlyricOffset(const qint64& offset);
	Q_INVOKABLE void saveTransparent(const float& transparent);
	Q_INVOKABLE void setPanelmodel(const int& model);

public:
	void randMusic();

public slots:
	void setCurrentMusicInfo();
	void setMusicLrc(const QString& musicPath);
public:
	QString getCurrentMusicName() const;
	QString getCurrentMusicSinger() const;
	QString getCurrentMusicCover() const { return currentMusicCover; };
	QUrl changedBg() const;
	QVariantList getmusicLrc() const;
	qint64 lrcPosition() const;
	qint64 getmusicPosition() const { return musicPlayer->position(); };
	qint64 getmusicTotalTime() const { return musicPlayer->duration(); };
	bool isPlaying()const;
	playStatus getPlayModel() const { return _playstatus; };
	QUrl getVideoPath() const { return _videoPath; };
	int getPlayPattern()const { return _playpattern; };
	void getMusicinfo(QString path);
	QUrl getPreviewPath() const { return _previewPath; }
	void loadSeetings();
	QString getLrcColor() const { return m_lrcColor; };
	QString getSpecColor() const { return m_spectrumColor; };
	qint64 getlyricOffset() const { return m_lyricOffset; };
	QString getCoverWithTimestamp(const QString& coverPath);
	float getTransprant() const { return m_transprant; };
	int getPanelmodel() const { return m_panelModel; };

	void setOnnxPath();

signals:
	void musicNameChanged();
	void musicSingerChanged();
	void musicCoverChanged();
	void bgchanged();
	void volumeChanged();
	void musicChanged();
	void musicAnalyzerChanged(QString musicPath);
	void musicLrcChanged();
	void musicPositionChanged();
	void analyzerPositionChanged(qint64 pos);
	void musicDurationChanged();
	void musicStatusChanged();
	void playStatusChanged();
	void videoPathChanged();
	void playModelsChanged();
	void startSurrounding(const QString path);
	void startOnnx(std::string path);
	void musicInfoReady(QString name, QString singer,QString cover);
	void previewpathChange();
	void lrcColorChanged();
	void spectrumColorChanged();
	void lyricOffsetChanged();
	void stopPreview();
	void transprantChanged();
	void panelModelChanged();

private:
	QAudioOutput* musicOutput;
	QMediaPlayer* musicPlayer;

	QUrl _videoPath;
	QUrl _previewPath;
	QUrl _bgPath;

	QVector<QString>musiclist;
	int currentMusicIndex = -1;

	QString currentMusicName;
	QString currentMusicSinger;
	QString currentMusicCover;

	QString m_lrcColor = "";
	QString m_spectrumColor = "";

	QVector<lrcLine> lrcList;

	playStatus _playstatus = playStatus::Sequential;
	int _playpattern = playModels::Normal;

	QString filePath = "";
	QString curMusicFileName = "";

	int curPos = 0;
	qint64 lastAnalyzerPos = 0;

	AudioSeparator* as = nullptr;
	AudioSeparator* asSurrounding = nullptr;
	QThread* thread1 = nullptr;
	QThread* thread2 = nullptr;

	float m_volume = 1.0;
	float m_transprant = 0.3f;

	QString m_taskName = "";
	QString onnxPath = "";

	//局部连接槽函数
	QMetaObject::Connection m_loadConnection;

	qint64 m_lyricOffset = 0;

	//页面model
	int m_panelModel = 0;

	//保存设置
	QSettings* settings;
	QTimer* saveTimer;
};
