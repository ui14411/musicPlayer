#include "HeaderFile/LoadlocalMusic.h"

#include <QDir>
#include <QFileInfo>
#include <QUrl>
#include <QFileSystemWatcher>
#include <iostream>
#include <QMediaMetaData>
#include <QMediaPlayer>

#include "HeaderFile/LoadlocalVideo.h"
#include "HeaderFile/PlayMusic.h"

loadlocalMusic::loadlocalMusic(QObject* parent,QString _path)
	: QAbstractListModel(parent),path(_path)
{
	if (onnxPath == "")
		setOnnxPath();

	as = new AudioSeparator();
	asSurrounding = new AudioSeparator();

	thread1 = new QThread(this);
	thread2 = new QThread(this);

	webgetCover = new WebgetCover();

	as->moveToThread(thread1);
	asSurrounding->moveToThread(thread2);

	connect(thread1, &QThread::finished, as, &QObject::deleteLater);
	connect(thread2, &QThread::finished, asSurrounding, &QObject::deleteLater);

	thread1->start();
	thread2->start();

	QMetaObject::invokeMethod(as, [=]() {as->loadModel(onnxPath.toStdString()); }, Qt::QueuedConnection);
	connect(this, &loadlocalMusic::startOnnx, as, &AudioSeparator::separate, Qt::QueuedConnection);

	connect(this, &loadlocalMusic::startSurrounding, asSurrounding, &AudioSeparator::Surrounding, Qt::QueuedConnection);

	connect(as, &AudioSeparator::sendtaskName, this, &loadlocalMusic::setTaskname);
	connect(as, &AudioSeparator::separateProgress, this, &loadlocalMusic::setProcess);
	connect(this, &loadlocalMusic::startDouble, asSurrounding, &AudioSeparator::doubleEarListening);

	connect(this, &loadlocalMusic::musicInfoReady, this, [this](QString name, QString singer)
	{
		musicName = name;
		musicSinger = singer;
	});

	QFileSystemWatcher* coverWatcher = new QFileSystemWatcher(this);
	QString coverDir = QCoreApplication::applicationDirPath() + "/cover/";
	QDir().mkpath(coverDir);
	coverWatcher->addPath(coverDir);

	connect(coverWatcher, &QFileSystemWatcher::directoryChanged, this, [this](const QString& path) {

		// 遍历列表，更新每个歌曲的封面路径（带时间戳）
		for (int i = 0; i < musiclist.size(); ++i) {
			QString musicName = musiclist[i]["musicName"].toString();
			QString coverPath = QCoreApplication::applicationDirPath() + "/cover/" + musicName + ".jpg";

			// 检查文件是否存在，如果存在则添加时间戳
			QFileInfo info(coverPath);
			QString newCover;
			if (info.exists()) {
				qint64 lastModified = info.lastModified().toMSecsSinceEpoch();
				newCover = QUrl::fromLocalFile(coverPath).toString() + "?t=" + QString::number(lastModified);
			}

			if (musiclist[i]["musicCover"].toString() != newCover) {
				musiclist[i]["musicCover"] = newCover;
				// 通知 UI 更新
				QModelIndex idx = index(i, 0);
				emit dataChanged(idx, idx, { MusicCoverRole });
			}
		}
		});

	QFileSystemWatcher* listWatcher = new QFileSystemWatcher(this);
	QString listDir = QCoreApplication::applicationDirPath() + "/music/";
	QDir().mkpath(listDir);
	listWatcher->addPath(listDir);
	connect(listWatcher, &QFileSystemWatcher::directoryChanged, this, [this](const QString& path) {
		loadMusiclist();
		});
}

loadlocalMusic::~loadlocalMusic()
{
	if (thread1)
	{
		thread1->quit();
		if (!thread1->wait(1000))
		{
			qDebug() << "thread1退出超时";
		}
	}
	if (thread2)
	{
		thread2->quit();
		if (!thread2->wait(1000))
		{
			qDebug() << "thread2退出超时";
		}
	}
}

void loadlocalMusic::loadMusiclist()
{
	musiclist.clear();

	beginResetModel();

	QDir dir(path);

	QStringList fileiter;
	fileiter << "*.mp3" << "*.wav" << "*.flac";

	QFileInfoList filelist = dir.entryInfoList(fileiter, QDir::Files);

	for (auto& list : filelist)
	{
		//こぴ - 白昼梦
		QString musicInfo = list.baseName();

		if (musicName == "" || musicSinger == "")
		{
			QStringList parts = musicInfo.split(" - ");

			if (parts.size() == 2)
			{
				musicName = musicInfo.split(" - ")[1];
				musicSinger = musicInfo.split(" - ")[0];
			}
			else
			{
				musicName = musicInfo;
				musicSinger = "unkonw";
			}
		}

		QVariantMap musicMap;

		musicMap["musicName"] = musicName;
		musicMap["musicSinger"] = musicSinger;
		musicMap["musicPath"] = list.filePath();
		musicName = "";
		musicSinger = "";

		int row = musiclist.size();
		musiclist.append(musicMap);

		getMusicinfo_loadmusicList(row, list.filePath());
	}

	endResetModel();
}

void loadlocalMusic::addMusic(const QString& musicPath)
{
	QString srcPath = QUrl(musicPath).toLocalFile();

	QFileInfo file(srcPath);

	QDir().mkpath(QCoreApplication::applicationDirPath() + "/music");

	QString targetPath = QCoreApplication::applicationDirPath() + "/music/" + file.fileName();
	 
	if (QFile(targetPath).exists())
	{
		return;
	}

	QVariantMap musicMap;

	if (musicName == "" || musicSinger == "")
	{
		QStringList part = file.completeBaseName().split(" - ");

		if (part.size() == 2)
		{
			musicMap["musicName"] = file.completeBaseName().split(" - ")[1];
			musicMap["musicSinger"] = file.completeBaseName().split(" - ")[0];
		}
		else if (part.size() == 1)
		{
			musicMap["musicName"] = file.completeBaseName().split(" - ")[0];
			musicMap["musicSinger"] = "unkonw";
		}
	}
	else
	{
		musicMap["musicName"] = musicName;
		musicMap["musicSinger"] = musicSinger;
	}
	musicMap["musicPath"] = targetPath;

	beginInsertRows(QModelIndex(), musiclist.size(), musiclist.size());

	musiclist.append(musicMap);

	getMusicinfo_addmusic(musiclist.size()-1, srcPath);

	endInsertRows();	
}

void loadlocalMusic::addMusicLrc(const QString& musicLrcPath,const QString& musicName)
{
	QString srcPath = QUrl(musicLrcPath).toLocalFile();

	QFileInfo fileInfo(musicName);

	QString target = QCoreApplication::applicationDirPath() + "/lrc/" + fileInfo.baseName() + ".lrc";

	if (QFile::exists(target))
	{
		QFile::remove(target);
	}

	QFile::copy(srcPath, target);

	emit musicLrcChanged(musicName);
}

void loadlocalMusic::addMusicCover(const QString& musicLrcPath, const QString& musicName)
{
	QString srcPath = QUrl(musicLrcPath).toLocalFile();
	qDebug() << "loadlocalMusic::addMusicCover:" << musicName;
	QFileInfo fileInfo(musicName);

	QString target = QCoreApplication::applicationDirPath() + "/cover/" + fileInfo.baseName() + ".jpg";

	qDebug() << "loadlocalMusic::addMusicCover target:" << target;

	if (QFile::exists(target))
	{
		QFile::remove(target);
	}

	QFile::copy(srcPath, target);

	emit musicCoverChanged(musicName);
}

void loadlocalMusic::playDimensionalMusic(const QString& l, const QString& r)
{
	
	QString leftMusic = QUrl(l).toLocalFile();
	QString rightMusic = QUrl(r).toLocalFile();

	QFileInfo Lname(leftMusic);
	QFileInfo Rname(rightMusic);

	QStringList Lparts = Lname.baseName().split(" - ");
	QString LmusicName;
	if (Lparts.size() == 2)
		LmusicName = Lname.baseName().split(" - ")[1];
	else
		LmusicName = Lname.baseName();

	QStringList Rparts = Rname.baseName().split(" - ");
	QString RmusicName;
	if (Rparts.size() == 2)
		RmusicName = Rname.baseName().split(" - ")[1];
	else
		RmusicName = Rname.baseName();

	emit startDouble(leftMusic, rightMusic, LmusicName, RmusicName);
}

void loadlocalMusic::replaceModel(const QString& filePath)
{
	QString modelDir = QCoreApplication::applicationDirPath() + "/model/";
	QDir().mkpath(modelDir);
	QString new_onnxPath = QFileInfo(filePath).fileName();
	QString targetPath = modelDir + new_onnxPath;

	QDir dir(modelDir);

	QStringList filters;
	filters << "*.onnx";

	QStringList old_onnxPaths = dir.entryList(filters,QDir::Files);
	QString old_onnxPath = modelDir + old_onnxPaths[0];
	if(!old_onnxPaths.isEmpty())
		QFile::remove(old_onnxPath);

	QFile::copy(filePath, targetPath);

	onnxPath = targetPath;
}

int loadlocalMusic::rowCount(const QModelIndex& parent) const
{
	return musiclist.size();
}

QVariant loadlocalMusic::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
		return {};

	auto& it = musiclist[index.row()];

	switch (role)
	{
		case MusicNameRole:
			return it["musicName"];
			break;
		case MusicPathRole:
			return it["musicPath"];
			break;
		case MusicSingerRole:
			return it["musicSinger"];
			break;
		case MusicCoverRole:
			return it["musicCover"];
			break;
		default:
			break;
	}

	return {};
}

QHash<int, QByteArray> loadlocalMusic::roleNames() const
{
	return 
	{
		{MusicNameRole, "musicName" },
		{ MusicPathRole, "musicPath" },
		{ MusicSingerRole, "musicSinger"},
		{MusicCoverRole, "musicCover" }
	};
}

void loadlocalMusic::removeMusic(int row)
{
	if (row < 0 || row >= musiclist.size())
		return;

	beginRemoveRows(QModelIndex(), row, row);

	QString removePath_music = musiclist.at(row)["musicPath"].toString();
	QFileInfo fileinfo(removePath_music);
	QString removefileName = fileinfo.completeBaseName();

	if(removefileName == m_taskName)
	{
		as->cancelTask();
		asSurrounding->cancelTask();
	}

	QString folder = QCoreApplication::applicationDirPath();
	QString removePath_video = findFile(folder+"/video/", removefileName);
	QString removePath_lrc = findFile(folder + "/lrc/", removefileName);
	QString removePath_pureVoice = findFile(folder + "/pureHumanVoice/", removefileName);
	QString removePath_pureAccompaniment = findFile(folder + "/pureAccompaniment/", removefileName);
	QString removePath_cover = findFile(folder + "/cover/", removefileName);

	QFile::remove(musiclist.at(row)["musicPath"].toString());

	if (!removePath_video.isEmpty())
	{
		QFile::remove(removePath_video);
	}
	if (!removePath_lrc.isEmpty())
	{
		QFile::remove(removePath_lrc);
	}
	if (!removePath_pureVoice.isEmpty())
	{
		QFile::remove(removePath_pureVoice);
	}
	if (!removePath_pureAccompaniment.isEmpty())
	{
		QFile::remove(removePath_pureAccompaniment);
	}
	if (!removePath_cover.isEmpty())
	{
		QFile::remove(removePath_cover);
	}

	musiclist.removeAt(row);

	endRemoveRows();
}

void loadlocalMusic::removeVideo(int row)
{
	if (row < 0 || row >= musiclist.size())
		return;

	beginRemoveRows(QModelIndex(), row, row);

	QString removePath_music = musiclist.at(row)["musicPath"].toString();
	QFileInfo fileinfo(removePath_music);
	QString removefileName = fileinfo.completeBaseName();
	QString folder = QCoreApplication::applicationDirPath();
	QString removePath_video = findFile(folder + "/video/", removefileName);

	if (!removePath_video.isEmpty())
	{
		QFile::remove(removePath_video);
	}
	musiclist.removeAt(row);

	endRemoveRows();
}

void loadlocalMusic::removelrc(int row)
{
	if (row < 0 || row >= musiclist.size())
		return;

	beginRemoveRows(QModelIndex(), row, row);

	QString removePath_music = musiclist.at(row)["musicPath"].toString();
	QFileInfo fileinfo(removePath_music);
	QString removefileName = fileinfo.completeBaseName();
	QString folder = QCoreApplication::applicationDirPath();
	QString reomvePath_lrc = findFile(folder + "/lrc/", removefileName);


	if (!reomvePath_lrc.isEmpty())
	{
		QFile::remove(reomvePath_lrc);
	}

	musiclist.removeAt(row);

	endRemoveRows();
}

void loadlocalMusic::setTaskname(QString path)
{
	m_taskName = path;

	emit taskNameChanged();
}

void loadlocalMusic::setProcess(int value)
{
	m_process = value;

	emit processChanged();
}

void loadlocalMusic::getMusicinfo_addmusic(int row,const QString& path)
{
	QMediaPlayer* temp = new QMediaPlayer(this);

	connect(temp, &QMediaPlayer::metaDataChanged, this,
		[this, row, path, temp]()
		{
			if (row < 0 || row >= musiclist.size())
			{
				temp->deleteLater();
				return;
			}

			auto meta = temp->metaData();
			QString title = meta.value(QMediaMetaData::Title).toString().trimmed();
			QString artist = meta.value(QMediaMetaData::ContributingArtist).toString().trimmed();
			if (artist.isEmpty())
				artist = meta.value(QMediaMetaData::Author).toString().trimmed();

			if (title.isEmpty() && artist.isEmpty()) 
			{
				temp->deleteLater();
				return;
			}

			QVariantMap& map = musiclist[row];
			bool changed = false;

			if (!title.isEmpty() && map["musicName"].toString() != title) 
			{
				map["musicName"] = title;
				changed = true;
			}
			if (!artist.isEmpty() && map["musicSinger"].toString() != artist) 
			{
				map["musicSinger"] = artist;
				changed = true;
			}
			QString targetPath = QCoreApplication::applicationDirPath() + "/music/" + title + ".mp3";

			if (QFile(targetPath).exists())
			{
				beginRemoveRows(QModelIndex(), row, row);
				
				endRemoveRows();

				return;
			}

			float targetDuration = temp->duration() / 1000.0;
			webgetCover->searchMusicInfo(title, artist, targetDuration);

			QString coverPath = QCoreApplication::applicationDirPath() + "/cover/" + title + ".jpg";
			map["musicPath"] = targetPath;
			map["musicCover"] = QUrl::fromLocalFile(coverPath).toString();

			QFile::copy(path, targetPath);

			emit startSurrounding(targetPath);
			emit startOnnx(targetPath.toStdString());

			if (changed) 
			{
				QModelIndex idx = index(row, 0);
				emit dataChanged(idx, idx, { MusicNameRole, MusicSingerRole,MusicCoverRole });
			}

			temp->deleteLater();
		});

	temp->setSource(QUrl::fromLocalFile(path));
}

void loadlocalMusic::getMusicinfo_loadmusicList(int row, const QString& path)
{
	QMediaPlayer* temp = new QMediaPlayer(this);

	connect(temp, &QMediaPlayer::metaDataChanged, this,
		[this, row, path, temp]()
		{
			if (row < 0 || row >= musiclist.size())
			{
				temp->deleteLater();
				return;
			}

			auto meta = temp->metaData();
			QString title = meta.value(QMediaMetaData::Title).toString().trimmed();
			QString artist = meta.value(QMediaMetaData::ContributingArtist).toString().trimmed();
			if (artist.isEmpty())
				artist = meta.value(QMediaMetaData::Author).toString().trimmed();

			if (title.isEmpty() && artist.isEmpty())
			{
				temp->deleteLater();
				return;
			}

			QVariantMap& map = musiclist[row];
			bool changed = false;

			if (!title.isEmpty() && map["musicName"].toString() != title)
			{
				map["musicName"] = title;
				changed = true;
			}
			if (!artist.isEmpty() && map["musicSinger"].toString() != artist)
			{
				map["musicSinger"] = artist;
				changed = true;
			}

			QString targetPath = QCoreApplication::applicationDirPath() + "/music/" + title + ".mp3";
			QString coverPath = QCoreApplication::applicationDirPath() + "/cover/" + title + ".jpg";
			map["musicPath"] = targetPath;
			qDebug() << "coverPath: " << coverPath;
			map["musicCover"] = QUrl::fromLocalFile(coverPath).toString();

			if (changed)
			{
				QModelIndex idx = index(row, 0);
				emit dataChanged(idx, idx, { MusicNameRole, MusicSingerRole,MusicCoverRole });
			}

			temp->deleteLater();
		});

	temp->setSource(QUrl::fromLocalFile(path));
}

QString loadlocalMusic::findFile(const QString& folder, const QString& baseName)
{
	QDir dir(folder);

	QStringList files = dir.entryList(
		QStringList(baseName + ".*"),
		QDir::Files);

	if (files.isEmpty())
		return "";

	return dir.absoluteFilePath(files.first());
}

void loadlocalMusic::setOnnxPath()
{
	QString modelDir = QCoreApplication::applicationDirPath() + "/model/";

	QDir dir(modelDir);

	QStringList filters;
	filters << "*.onnx";

	QStringList old_onnxPaths = dir.entryList(filters, QDir::Files);
	QString old_onnxPath = modelDir + old_onnxPaths[0];

	onnxPath = old_onnxPath;
}
