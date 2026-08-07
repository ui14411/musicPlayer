#include <QDir>
#include <QFileInfo>
#include <QUrl>
#include <QCoreApplication>
#include <QFileSystemWatcher>
#include <iostream>
#include <QMediaMetaData>
#include <QMediaPlayer>

#include "HeaderFile/DoubleEar.h"

DoubleEar::DoubleEar(QObject* parent, QString _path)
	: QAbstractListModel(parent), path(_path)
{	
	QFileSystemWatcher* watcher = new QFileSystemWatcher(this);

	watcher->addPath(path);

	connect(watcher, &QFileSystemWatcher::directoryChanged, this, &DoubleEar::loadMusiclist);
}

DoubleEar::~DoubleEar()
{}

void DoubleEar::loadMusiclist()
{
	beginResetModel();

	musiclist.clear();

	QDir dir(path);

	if (!dir.exists())
	{
		qDebug() << "double music path error";
		endResetModel();
		return;
	}

	// 获取子文件夹
	QFileInfoList folderList = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

	for (auto& folder : folderList)
	{

		QString folderName =
			folder.fileName();

		QVariantMap musicMap;

		// 双耳音乐名字
		musicMap["musicName"] = folderName;

		// 占位歌手
		musicMap["musicSinger"] = "Double Ear";

		musicMap["musicPath"] = folder.absoluteFilePath();

		musiclist.append(musicMap);
	}
	endResetModel();
}

int DoubleEar::rowCount(const QModelIndex& parent) const
{
	return musiclist.size();
}

QVariant DoubleEar::data(const QModelIndex& index, int role) const
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
		default:
			break;
	}

	return {};
}

QHash<int, QByteArray> DoubleEar::roleNames() const
{
	return
	{
		{MusicNameRole, "musicName" },
		{ MusicPathRole, "musicPath" },
		{ MusicSingerRole, "musicSinger"}
	};
}

void DoubleEar::removeMusic(int row)
{
	if (row < 0 || row >= musiclist.size())
		return;

	beginRemoveRows(QModelIndex(), row, row);

	QString folder = musiclist.at(row)["musicPath"].toString();

	QDir dir(folder);

	if (dir.exists())
	{
		dir.removeRecursively();
	}

	musiclist.removeAt(row);

	endRemoveRows();
}

QVariantList DoubleEar::getAllPaths() const
{
	QVariantList paths;
	for (const auto& item : musiclist) {
		paths.append(item["musicPath"]);
	}
	return paths;
}