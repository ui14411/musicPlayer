#include "HeaderFile/PlayMusic.h"

#include <QDir>
#include <iostream>
#include <QCoreApplication>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTimer>
#include <QRandomGenerator>
#include <QFile>
#include <QFileSystemWatcher>
#include <QMediaMetaData>

PlayMusic::PlayMusic(QObject* parent,QString _filePath)
	: QObject(parent),filePath(_filePath)
{
	if (!onnxPath.isEmpty())
		setOnnxPath();

	musicPlayer = new QMediaPlayer(this);
	musicOutput = new QAudioOutput(this);
	musicPlayer->setAudioOutput(musicOutput);

	as = new AudioSeparator();
	asSurrounding = new AudioSeparator();

	saveTimer = new QTimer(this);

	QString configPath = QCoreApplication::applicationDirPath()
		+ "/config/setting.ini";

	QDir().mkpath(QCoreApplication::applicationDirPath() + "/config");

	settings = new QSettings(configPath, QSettings::IniFormat,this);
	loadSeetings();
	musicOutput->setVolume(m_volume);

	connect(saveTimer, &QTimer::timeout, this, [=]()
	{
		settings->setValue("volume", m_volume);
		settings->setValue("transprant", m_transprant);
		settings->setValue("pos", curPos);
	});

	thread1 = new QThread(this);
	thread2 = new QThread(this);

	as->moveToThread(thread1);
	asSurrounding->moveToThread(thread2);

	connect(thread1, &QThread::finished, as, &QObject::deleteLater);
	connect(thread2, &QThread::finished, asSurrounding, &QObject::deleteLater);

	thread1->start();
	thread2->start();

	QMetaObject::invokeMethod(as, [=]() {as->loadModel(onnxPath.toStdString()); }, Qt::QueuedConnection);
	connect(this, &PlayMusic::startOnnx, as, &AudioSeparator::separate, Qt::QueuedConnection);

	connect(this, &PlayMusic::startSurrounding, asSurrounding, &AudioSeparator::Surrounding, Qt::QueuedConnection);

	QFileSystemWatcher* watcher = new QFileSystemWatcher(this);

	watcher->addPath(filePath);

	connect(watcher, &QFileSystemWatcher::directoryChanged, this, &PlayMusic::setMusiclist);

	QTimer* timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, [=] {
		emit musicLrcChanged();
		});
	timer->start(100);

	connect(musicPlayer, &QMediaPlayer::mediaStatusChanged, this,
		[this](QMediaPlayer::MediaStatus status) {
			if (status == QMediaPlayer::EndOfMedia)
				setMusicModel();
		});

	connect(musicPlayer, &QMediaPlayer::durationChanged, this, [this](qint64 duration) {
		emit musicDurationChanged();
		});

	connect(musicPlayer, &QMediaPlayer::playbackStateChanged, this, [this]() {
		emit musicStatusChanged();
		});

	connect(this, &PlayMusic::musicInfoReady, this, [this](QString name, QString singer,QString cover)
	{
		currentMusicName = name;
		currentMusicSinger = singer;
		currentMusicCover = cover;
		QString coverPath = QCoreApplication::applicationDirPath() + "/cover/" + currentMusicName + ".jpg";
		currentMusicCover = getCoverWithTimestamp(coverPath);
	});

	connect(musicPlayer, &QMediaPlayer::positionChanged, this, [this](qint64 pos) 
	{
		emit musicPositionChanged();
		if (pos - lastAnalyzerPos > 50 || lastAnalyzerPos - pos > 50)
		{
			emit analyzerPositionChanged(pos);

			lastAnalyzerPos = pos;
		}
	});

	connect(this, &PlayMusic::stopPreview, this, [this](QString path = "") {
		this->_previewPath = "";
		emit previewpathChange();
		});

	QFileSystemWatcher* coverWatcher = new QFileSystemWatcher(this);
	QString coverDir = QCoreApplication::applicationDirPath() + "/cover/";
	QDir().mkpath(coverDir);
	coverWatcher->addPath(coverDir);

	connect(coverWatcher, &QFileSystemWatcher::directoryChanged, this, [this](const QString& path) {
		qDebug() << "封面目录发生变化:" << path;
		if (!currentMusicName.isEmpty()) {
			QString coverPath = QCoreApplication::applicationDirPath() + "/cover/" + currentMusicName + ".jpg";
			QString newCover = getCoverWithTimestamp(coverPath);
			if (newCover != currentMusicCover) {
				currentMusicCover = newCover;
				emit musicCoverChanged();
				qDebug() << "封面已自动刷新";
			}
		}
		});
}

PlayMusic::~PlayMusic()
{
	if (thread1)
	{
		thread1->quit();
		thread1->wait();
	}


	if (thread2)
	{
		thread2->quit();
		thread2->wait();
	}
}

void PlayMusic::playMusic(const QString & musicPath)
{
	emit musicPositionChanged();
	QFileInfo info(musicPath);
	qDebug() << "PlayMusic::playMusic:" << musicPath;
	if(!musicPath.isEmpty())
		musicPlayer->play();

	if (curMusicFileName != info.fileName())
	{
		setPlayPattern(0);
		musicPlayer->setSource(QUrl::fromLocalFile(musicPath));
		qDebug() << "PlayMusic::playMusic:" << musicPath;
		musicPlayer->play();
		musicAnalyzerChanged(musicPath);
		emit analyzerPositionChanged(0);
		emit musicAnalyzerChanged(musicPath);
	}
	
	setMusicLrc(musicPath);

	currentMusicIndex = musiclist.indexOf(QFileInfo(musicPath).absoluteFilePath());
	curMusicFileName = info.fileName();
	settings->setValue("musicPath",musicPath);

	setCurrentMusicInfo();
}

void PlayMusic::playVideo(const QString& videoPath)
{
	if (videoPath.isEmpty())
	{	
		this->_videoPath = "";
		emit videoPathChanged();
		return;
	}
	QString musicName = QFileInfo(videoPath).completeBaseName();

	QFileInfo fileinfo(videoPath);
	QString videoName = fileinfo.completeBaseName();
	if (videoName.isEmpty()) videoName = musicName;
	videoName += ".mp4";

	this->_videoPath = QUrl::fromLocalFile(
		QCoreApplication::applicationDirPath() + "/video/" + videoName
	);
	qDebug() << this->_videoPath;
	emit videoPathChanged();
}

void PlayMusic::setVolume(const float& value)
{
	if (m_volume == value) return;
	
	m_volume = value;

	musicOutput->setVolume(m_volume);

	saveTimer->start(1000);

	emit volumeChanged();
}

float PlayMusic::volumce() const
{
	return m_volume;
}

void PlayMusic::setMusiclist()
{
	musiclist.clear();

	QDir dir(filePath);

	QStringList fileiter;
	fileiter << "*.mp3" << "*.wav" << "*.flac";

	QFileInfoList filelist = dir.entryInfoList(fileiter, QDir::Files);
	for (const auto& list:filelist)
	{
		QString path = QCoreApplication::applicationDirPath() + "/music/" + list.fileName();
		musiclist.append(path);
	}

	QString last_musicPath = settings->value("musicPath", "").toString();

	if (!last_musicPath.isEmpty()&& musiclist.contains(last_musicPath))
	{
		setPlayModel(_playstatus);
		playMusic(last_musicPath);
		setPosition(curPos);
		Pause();
	}
}

void PlayMusic::setNormMusiclist(const QVariantList& paths)
{
	musiclist.clear();
	musiclist.reserve(paths.size());
	for (const QVariant& v : paths)
	{
		musiclist.append(v.toString());
	}

	currentMusicIndex = musiclist.isEmpty() ? -1 : 0;
	if (currentMusicIndex == -1)
		return;
	setPlayModel(_playstatus);
}

QString PlayMusic::getCurrentMusicName() const
{
	return currentMusicName;
}

QString PlayMusic::getCurrentMusicSinger() const
{
	return currentMusicSinger;
}

void PlayMusic::setCurrentMusicInfo()
{
	if (currentMusicIndex < 0 || currentMusicIndex >= musiclist.size())
		return;
	getMusicinfo(musiclist[currentMusicIndex]);

	if(currentMusicName == "" || currentMusicSinger == "")
	{
		QStringList list = QFileInfo(musiclist[currentMusicIndex]).baseName().split(" - ");
		if (list.size() == 2)
		{
			currentMusicName = QFileInfo(musiclist[currentMusicIndex]).baseName().split(" - ")[1];
			currentMusicSinger = QFileInfo(musiclist[currentMusicIndex]).baseName().split(" - ")[0];
		}
		else
		{
			currentMusicName = list[0];
			currentMusicSinger = "No Singer";
		}
	}

	QString coverPath = QCoreApplication::applicationDirPath() + "/cover/" + currentMusicName + ".jpg";
	currentMusicCover = getCoverWithTimestamp(coverPath);  

	emit musicNameChanged();
	emit musicSingerChanged();
	emit musicCoverChanged();
}

void PlayMusic::setbgImage(const QUrl& bgPath)
{
	QString _bgPath = QUrl(bgPath).toLocalFile();
	
	this->_bgPath = bgPath;

	settings->setValue("background", this->_bgPath);

	emit bgchanged();
}

void PlayMusic::addbgImage(const QUrl& bgPath)
{
	QString filePath = QCoreApplication::applicationDirPath() + "/Image/";

	QString _bgPath = QUrl(bgPath).toLocalFile();

	QDir().mkpath(filePath);

	QString targetPath = filePath + QFileInfo(_bgPath).fileName();

	bool ok = QFile::copy(_bgPath, targetPath);

	setbgImage();
}

void PlayMusic::setbgImage()
{
	if (!this->_bgPath.isEmpty() && QFile(this->_bgPath.toString()).exists())
	{
		return;
	}
	QString filePath = QCoreApplication::applicationDirPath() + "/Image/";

	QDir().mkpath(filePath);

	QDir dir(filePath);

	QFileInfoList filelist = dir.entryInfoList(QDir::Files);

	if (filelist.isEmpty())
	{
		this->_bgPath = "";
		emit bgchanged();
		return;
	}

	std::sort(filelist.begin(), filelist.end(),
		[](const QFileInfo& a, const QFileInfo& b)
		{
			return a.birthTime() > b.birthTime();
		});

	this->_bgPath = filelist[0].absoluteFilePath();
	settings->setValue("background", this->_bgPath);
	emit bgchanged();
}

void PlayMusic::setMusicLrc(const QString& musicPath)
{
	lrcList.clear();

	QString lrcPath = QCoreApplication::applicationDirPath() + "/lrc/" + QFileInfo(musicPath).completeBaseName() + ".lrc";

	QFile file(lrcPath);

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qDebug() << "[PlayMusic::setMusicLrc]open failed";
		emit musicChanged();
		return;
	}

	QTextStream in(&file);
	in.setEncoding(QStringConverter::Utf8);

	while (!in.atEnd())
	{
		QString line = in.readLine().trimmed();
		if (line.isEmpty()) continue;

		// JSON
		if (line.startsWith("{"))
		{
			QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8());
			QJsonObject obj = doc.object();

			qint64 time = obj["t"].toInt();
			QJsonArray content = obj["c"].toArray();

			QString text;
			for (auto item : content)
				text += item.toObject()["tx"].toString();

			lrcList.push_back({ time, text });
		}
		// LRC
		else if (line.startsWith("["))
		{
			QRegularExpression re(
				"^\\[(\\d+):(\\d+\\.\\d+)\\](.*)$"
			);

			auto match = re.match(line);

			if (match.hasMatch())
			{
				int m = match.captured(1).toInt();
				double s = match.captured(2).toDouble();

				lrcList.push_back({
					m * 60000 + (qint64)(s * 1000),
					match.captured(3).trimmed()
					});
			}
		}
	}

	std::sort(lrcList.begin(), lrcList.end(),
		[](const lrcLine& a, const lrcLine& b)
		{
			return a.time < b.time;
		});

	emit musicChanged();
}

void PlayMusic::setPosition(const qint64& pos)
{
	musicPlayer->setPosition(pos);
	emit analyzerPositionChanged(pos);
	saveTimer->start(1000);
}

void PlayMusic::Pause()
{
	musicPlayer->pause();
}

void PlayMusic::Play()
{
	musicPlayer->play();
}

void PlayMusic::nextMusic()
{
	currentMusicIndex = (currentMusicIndex + 1) % musiclist.size();
	QString path = musiclist[currentMusicIndex];
	playMusic(path);
	playVideo(path);
}

void PlayMusic::prevMusic()
{
	currentMusicIndex = (currentMusicIndex - 1 + musiclist.size()) % musiclist.size();
	QString path = musiclist[currentMusicIndex];
	playMusic(path);
	playVideo(path);
}

void PlayMusic::randMusic()
{
	int randomIndex = QRandomGenerator::global()->bounded(musiclist.size());

	if (musiclist.size() > 1 && randomIndex == currentMusicIndex) 
	{
	     randomIndex = (randomIndex + 1) % musiclist.size();
	}

	currentMusicIndex = randomIndex;

	QString path = musiclist[currentMusicIndex];

	playMusic(path);
	playVideo(path);
}

void PlayMusic::setPlayModel(playStatus playstatus)
{
	if (playstatus == _playstatus)
		return;
	_playstatus = playstatus;
	settings->setValue("playstatus", playstatus);
	emit playStatusChanged();
}

void PlayMusic::switchModel()
{
	switch (_playstatus)
	{
		case PlayMusic::RepeatOne:
			setPlayModel(Random);
			break;
		case PlayMusic::Random:
			setPlayModel(Sequential);
			break;
		case PlayMusic::Sequential:
			setPlayModel(RepeatOne);
			break;
		default:
			break;
	}
}

void PlayMusic::setMusicModel()
{
	if (musiclist.isEmpty()) return;

	switch (_playstatus)
	{
	case PlayMusic::RepeatOne:
		if (currentMusicIndex == -1)
			currentMusicIndex = 0;
		playMusic(musiclist[currentMusicIndex]);
		playVideo(musiclist[currentMusicIndex]);
		break;
	case PlayMusic::Random:
		if (musiclist.size() == 1)
			_playstatus = RepeatOne;
		else
			randMusic();
		break;
	case PlayMusic::Sequential:
		nextMusic();
		break;
	default:
		break;
	}
}

void PlayMusic::setPlayPattern(int pattern)
{
	if (_playpattern == pattern)
		return;

	_playpattern = pattern;

	switchPattern();

	emit playModelsChanged();
}

void PlayMusic::switchPattern()
{
	curPos = musicPlayer->position();
	musicPlayer->stop();
	switch (_playpattern)
	{
	case 0:
		playNormMusic();
		break;
	case 1:
		playPureVoiceMusic();
		break;
	case 2:
		playAccompanimentMusic();
		break;
	case 3:
		playSurroundingMusic();
		break;
	default:
		break;
	}
}

void PlayMusic::playNormMusic()
{
	QString path = QCoreApplication::applicationDirPath() + "/music/" + curMusicFileName;

	if (m_loadConnection)
		disconnect(m_loadConnection);

	m_loadConnection = connect(musicPlayer, &QMediaPlayer::mediaStatusChanged, this,
		[this](QMediaPlayer::MediaStatus status) {
			if (status == QMediaPlayer::LoadedMedia || status == QMediaPlayer::BufferedMedia) {
				// 断开自身，防止再次触发
				disconnect(m_loadConnection);
				musicPlayer->setPosition(curPos);
				musicPlayer->play();
				emit musicPositionChanged();
			}
		});
	musicPlayer->setSource(path);
}

void PlayMusic::playPureVoiceMusic()
{
	QString path = QCoreApplication::applicationDirPath() + "/pureHumanVoice/" + curMusicFileName;

	std::string pathStr = (QCoreApplication::applicationDirPath() + "/music/" + curMusicFileName).toStdString();

	if(!QFile(path).exists())
	{
		emit startOnnx(pathStr);
		return;
	}
	else
	{
		musicPlayer->setSource(path);

		if (m_loadConnection)
			disconnect(m_loadConnection);

		m_loadConnection = connect(musicPlayer, &QMediaPlayer::mediaStatusChanged, this,
			[this](QMediaPlayer::MediaStatus status) {
				if (status == QMediaPlayer::LoadedMedia || status == QMediaPlayer::BufferedMedia) {
					// 断开自身，防止再次触发
					disconnect(m_loadConnection);
					musicPlayer->setPosition(curPos);
					musicPlayer->play();
					emit musicPositionChanged();
				}
			});
	}
}

void PlayMusic::playAccompanimentMusic()
{
	QString path = QCoreApplication::applicationDirPath() + "/pureAccompaniment/" + curMusicFileName;

	std::string pathStr = (QCoreApplication::applicationDirPath() + "/music/" + curMusicFileName).toStdString();

	if (!QFile(path).exists())
	{
		emit startOnnx(pathStr);
		return;
	}
	else
	{
		musicPlayer->setSource(path);

		if (m_loadConnection)
			disconnect(m_loadConnection);

		m_loadConnection = connect(musicPlayer, &QMediaPlayer::mediaStatusChanged, this,
			[this](QMediaPlayer::MediaStatus status) {
				if (status == QMediaPlayer::LoadedMedia || status == QMediaPlayer::BufferedMedia) {
					// 断开自身，防止再次触发
					disconnect(m_loadConnection);
					musicPlayer->setPosition(curPos);
					musicPlayer->play();
					emit musicPositionChanged();
				}
			});
	}
}

void PlayMusic::playSurroundingMusic()
{
	QString path = QCoreApplication::applicationDirPath() + "/surrounding/" + curMusicFileName;

	QString pathStr = QCoreApplication::applicationDirPath() + "/music/" + curMusicFileName;

	if (!QFile(path).exists())
	{
		emit startSurrounding(pathStr);
		return;
	}
	else
	{
		musicPlayer->setSource(path);

		if (m_loadConnection)
			disconnect(m_loadConnection);

		m_loadConnection = connect(musicPlayer, &QMediaPlayer::mediaStatusChanged, this,
			[this](QMediaPlayer::MediaStatus status) {
				if (status == QMediaPlayer::LoadedMedia || status == QMediaPlayer::BufferedMedia) {
					disconnect(m_loadConnection);
					musicPlayer->setPosition(curPos);
					musicPlayer->play();
					emit musicPositionChanged();
				}
			});
	}
}

void PlayMusic::playPreviewMusic(const QString& videoPath)
{
	if (videoPath.isEmpty())
	{
		emit stopPreview();
		return;
	}
	QString musicName = QFileInfo(videoPath).completeBaseName();

	QFileInfo fileinfo(videoPath);
	QString videoName = fileinfo.completeBaseName();
	if (videoName.isEmpty()) videoName = musicName;
	videoName += ".mp4";

	this->_previewPath = QUrl::fromLocalFile(QCoreApplication::applicationDirPath() + "/video/" + videoName);
	emit previewpathChange();
}

void PlayMusic::playPreviewImage(const QString& imagePath)
{
	if (imagePath.isEmpty())
	{
		this->_previewPath = "";
		emit previewpathChange();
		return;
	}
	
	this->_previewPath = imagePath;

	emit previewpathChange();
}

void PlayMusic::stopMusic()
{
	if (musicPlayer)
	{
		musicPlayer->stop();
		musicPlayer->setSource(QUrl());
		currentMusicName = "";
		currentMusicSinger = "";
		currentMusicCover = "";
		emit musicNameChanged();
		emit musicSingerChanged();
		emit musicCoverChanged();
	}
}

bool PlayMusic::isPlaying() const
{
	return musicPlayer && musicPlayer->playbackState() == QMediaPlayer::PlayingState;
}

QUrl PlayMusic::changedBg() const
{
	return _bgPath;
}

QVariantList PlayMusic::getmusicLrc() const
{
	QVariantList list;
	
	for (const auto& lrc : lrcList)
	{
		QVariantMap map;
		map["time"] = lrc.time;
		map["text"] = lrc.text;
		list.append(map);
 	}

	return list;
}

qint64 PlayMusic::lrcPosition() const
{
	return musicPlayer->position();
}

void PlayMusic::getMusicinfo(QString path)
{
	QMediaPlayer* temp = new QMediaPlayer(this);

	connect(temp, &QMediaPlayer::metaDataChanged, this, [this, temp]()
	{
		auto meta = temp->metaData();

		QString name = meta.value(QMediaMetaData::Title).toString();
		QString singer = meta.value(QMediaMetaData::ContributingArtist).toString();
		QString cover = QCoreApplication::applicationDirPath() + "/cover/" + name + ".jpg";

		emit musicInfoReady(name, singer,cover);
		emit musicNameChanged();
		emit musicSingerChanged();
		emit musicCoverChanged();

		temp->deleteLater();
	});

	temp->setSource(QUrl::fromLocalFile(path));
}

void PlayMusic::loadSeetings()
{
	m_volume = settings->value("volume",1.0).toFloat();
	curPos = settings->value("pos", 0).toInt();
	_playstatus = static_cast<playStatus>(settings->value("playstatus", Sequential).toInt());
	m_lrcColor = settings->value("lrcColor", "purple").toString();
	m_spectrumColor = settings->value("sepctrumColor", "purple").toString();
	_bgPath = settings->value("background", "").toString();
	m_transprant = settings->value("transprant", 0.3).toFloat();
}

QString PlayMusic::getCoverWithTimestamp(const QString& coverPath)
{
	QFileInfo info(coverPath);
	if (!info.exists()) {
		return "";
	}
	qint64 lastModified = info.lastModified().toMSecsSinceEpoch();
	return coverPath + "?t=" + QString::number(lastModified);
}

QVariantList PlayMusic::getAllPaths() const
{
	QVariantList paths;
	for (const auto& item : musiclist) 
	{
		paths.append(item);
	}
	return paths;
}

void PlayMusic::setLrcColor(const QString& color)
{
	m_lrcColor = color;
	emit lrcColorChanged();
	settings->setValue("lrcColor", m_lrcColor);
}

void PlayMusic::setSpectrumColor(const QString& color)
{
	m_spectrumColor = color;
	emit spectrumColorChanged();
	settings->setValue("sepctrumColor", m_spectrumColor);
}

void PlayMusic::setlyricOffset(const qint64& offset)
{
	if (m_lyricOffset == offset)
		return;

	m_lyricOffset = offset;
	emit lyricOffsetChanged();
}

void PlayMusic::saveTransparent(const float& transparent)
{
	m_transprant = transparent;

	saveTimer->start(1000);

	emit transprantChanged();
}

void PlayMusic::setPanelmodel(const int& model)
{
	this->m_panelModel = model;

	emit panelModelChanged();
}
void PlayMusic::setOnnxPath()
{
	QString modelDir = QCoreApplication::applicationDirPath() + "/model/";

	QDir dir(modelDir);

	QStringList filters;
	filters << "*.onnx";

	QStringList old_onnxPaths = dir.entryList(filters, QDir::Files);
	QString old_onnxPath = modelDir + old_onnxPaths[0];

	onnxPath = old_onnxPath;
}