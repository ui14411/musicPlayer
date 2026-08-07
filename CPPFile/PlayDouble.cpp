#include "HeaderFile/PlayDouble.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>

PlayDouble::PlayDouble(QObject* parent)
	:QObject(parent)
{
	leftPlayer = new QMediaPlayer(this);
	rightPlayer = new QMediaPlayer(this);
	leftAudioOutput = new QAudioOutput(this);
	rightAudioOutput = new QAudioOutput(this);
	leftPlayer->setAudioOutput(leftAudioOutput);
	rightPlayer->setAudioOutput(rightAudioOutput);

	connect(leftPlayer, &QMediaPlayer::durationChanged, this, [this](qint64 duration) 
	{
		emit leftTotaltimeChanged();
	});

	connect(rightPlayer, &QMediaPlayer::durationChanged, this, [this](qint64 duration) 
	{
		emit rightTotaltimeChanged();
	});

	connect(leftPlayer, &QMediaPlayer::positionChanged, this, [this](qint64 pos)
	{
		m_leftPosition = pos;
		emit leftPosChanged();
	});

	connect(rightPlayer, &QMediaPlayer::positionChanged, this, [this](qint64 pos)
	{
		m_rightPosition = pos;
		emit rightPosChanged();
	});

	connect(leftPlayer, &QMediaPlayer::playbackStateChanged, this, [this]() 
	{
		emit leftMusicStatusChanged();
	});

	connect(rightPlayer, &QMediaPlayer::playbackStateChanged, this, [this]()
	{
		emit rightMusicStatusChanged();
	});
}

void PlayDouble::loadDoubleMusic(const QString& path)
{
	QDir dir(path);
	QStringList paths = dir.entryList(QStringList() << "*.wav", QDir::Files);
	if (paths.size() != 2)
	{
		qDebug() << "[PlayDouble::loadDoubleMusic]" + path;
		return;
	}
	currentLeftMusicPath = dir.absoluteFilePath(paths[0]);
	currentRightMusicPath = dir.absoluteFilePath(paths[1]);
	QStringList names = dir.dirName().split("+");
	if (names.size() != 2)
	{
		qDebug() << "[PlayDouble::loadDoubleMusic] names is failed" + path;
		return;
	}
	currentLeftMusicName = names[0];
	currentRightMusicName = names[1];

	currentLeftMusicCover = QCoreApplication::applicationDirPath() + "/cover/" + currentLeftMusicName + ".jpg";
	currentRightMusicCover = QCoreApplication::applicationDirPath() + "/cover/" + currentRightMusicName + ".jpg";

	m_videoPath = QCoreApplication::applicationDirPath() + "/video/" + currentLeftMusicName + ".mp4";
	if(!QFile(m_videoPath).exists())
	{
		m_videoPath = QCoreApplication::applicationDirPath() + "/video/" + currentRightMusicName + ".mp4";
		emit videoPathChanged();
	}
	
	setleftMusicLrc(currentLeftMusicName);
	setrightMusicLrc(currentRightMusicName);

	playLeftMusic();
	playRightMusic();

	emit leftMusicPathChanged();
	emit rightMusicPathChanged();
	emit leftMusicNameChanged();
	emit rightMusicNameChanged();
	emit leftCoverChanged();
	emit rightCoverChanged();
}

void PlayDouble::setLeftVolume(const float& volume)
{
	this->m_leftVolume = volume;
	leftAudioOutput->setVolume(volume);
	emit leftVolumeChanged();
}

void PlayDouble::setRightVolume(const float& volume)
{
	this->m_rightVolume = volume;
	rightAudioOutput->setVolume(volume);
	emit rightVolumeChanged();
}

void PlayDouble::setLeftPos(const int& pos)
{
	this->m_leftPosition = pos;
	leftPlayer->setPosition(pos);
}

void PlayDouble::setRightPos(const int& pos)
{
	this->m_rightPosition = pos;
	rightPlayer->setPosition(pos);
}

void PlayDouble::playLeftMusic()
{
	leftPlayer->setSource(QUrl::fromLocalFile(currentLeftMusicPath));
	leftPlayer->play();
}

void PlayDouble::playRightMusic()
{
	rightPlayer->setSource(QUrl::fromLocalFile(currentRightMusicPath));
	rightPlayer->play();
}

void PlayDouble::stopLeftMusic()
{
	leftPlayer->pause();
}

void PlayDouble::stopRightMusic()
{
	rightPlayer->pause();
}

QVariantList PlayDouble::getleftLrc() const
{
	QVariantList list;

	for (const auto& lrc : m_leftLrc)
	{
		QVariantMap map;
		map["time"] = lrc.time;
		map["text"] = lrc.text;
		list.append(map);
	}

	return list;
}

QVariantList PlayDouble::getRightLrc() const
{
	QVariantList list;
	for (const auto& lrc : m_rightLrc)
	{
		QVariantMap map;
		map["time"] = lrc.time;
		map["text"] = lrc.text;
		list.append(map);
	}

	return list;
}

void PlayDouble::setleftMusicLrc(const QString& musicName)
{
	m_leftLrc.clear();

	QString lrcPath = QCoreApplication::applicationDirPath() + "/lrc/" + musicName + ".lrc";
	qDebug() << "lrcPath:" << lrcPath;
	QFile file(lrcPath);

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qDebug() << "[PlayDouble::setleftMusicLrc]open failed";
		emit leftLrcChanged();
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

			m_leftLrc.push_back({ time, text });
		}
		// LRC
		else if (line.startsWith("["))
		{
			QRegularExpression re("^\\[(\\d+):(\\d+\\.\\d+)\\](.*)$");

			auto match = re.match(line);

			if (match.hasMatch())
			{
				int m = match.captured(1).toInt();
				double s = match.captured(2).toDouble();

				m_leftLrc.push_back({ m * 60000 + (qint64)(s * 1000),match.captured(3).trimmed() });
			}
		}
	}

	std::sort(m_leftLrc.begin(), m_leftLrc.end(),
		[](const lrcLine& a, const lrcLine& b)
		{
			return a.time < b.time;
		});

	emit leftLrcChanged();
}

void PlayDouble::setrightMusicLrc(const QString& musicName)
{
	m_rightLrc.clear();

	QString lrcPath = QCoreApplication::applicationDirPath() + "/lrc/" + musicName + ".lrc";

	QFile file(lrcPath);

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qDebug() << "[PlayDouble::setleftMusicLrc]open failed";
		emit rightLrcChanged();
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

			m_rightLrc.push_back({ time, text });
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

				m_rightLrc.push_back({
					m * 60000 + (qint64)(s * 1000),
					match.captured(3).trimmed()
					});
			}
		}
	}

	std::sort(m_rightLrc.begin(), m_rightLrc.end(),
		[](const lrcLine& a, const lrcLine& b)
		{
			return a.time < b.time;
		});

	emit rightLrcChanged();
}

