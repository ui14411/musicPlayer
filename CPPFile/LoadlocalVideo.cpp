#include "HeaderFile/LoadlocalVideo.h"

#include <QDir>
#include <QFileInfo>
#include <QUrl>
#include <QCoreApplication>
#include <iostream>

loadlocalVideo::loadlocalVideo(QObject* parent)
	: QAbstractListModel(parent)
{
}

void loadlocalVideo::loadVideoList(QString path)
{
	videolist.clear();

	beginResetModel();

	QDir dir(path);

	QStringList fileiter;
	fileiter << "*.mp4" << "*.mkv" << "*.avi" << "*.mov";

	QFileInfoList filelist = dir.entryInfoList(fileiter, QDir::Files);

	for (auto& list : filelist)
	{
		QVariantMap videoMap;
		videoMap["videoName"] = list.completeBaseName();
		videoMap["videoPath"] = list.filePath();

		videolist.append(videoMap);
	}

	endResetModel();
}

void loadlocalVideo::addVideo(const QString& videoPath,const QString& musicName)
{
	QString srcPath = QUrl(videoPath).toLocalFile();

	QFileInfo file(srcPath);
	QFileInfo musicfile(musicName);

	QDir().mkpath(QCoreApplication::applicationDirPath() + "/video");

	QString targetPath = QCoreApplication::applicationDirPath() + "/video/" + musicfile.completeBaseName() + ".mp4";

	if (QFile::exists(targetPath))
	{
		emit addVideoFailed("this video already exists");
		return;
	}

	videoName = musicName;
	pendingPath = targetPath;

	qDebug() << "targetPath:" + pendingPath;

	QFile::copy(srcPath, pendingPath);

	QVariantMap videoMap;
	videoMap["videoName"] = videoName;
	videoMap["videoPath"] = pendingPath;

	beginInsertRows(QModelIndex(), videolist.size(), videolist.size());

	videolist.append(videoMap);

	endInsertRows();
}

int loadlocalVideo::rowCount(const QModelIndex& parent) const
{
	return videolist.size();
}

QVariant loadlocalVideo::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
		return {};

	auto& it = videolist.at(index.row());

	switch (role)
	{
		case VideoNameRoles:
			return it["videoName"];
			break;
		case videoPathRoles:
			return it["videoPath"];
			break;
		default:
			break;
	}
	
	return {};
}

QHash<int, QByteArray> loadlocalVideo::roleNames() const
{
	return
	{
		{VideoNameRoles,"videoName"},
		{videoPathRoles,"videoPath"}
	};
}