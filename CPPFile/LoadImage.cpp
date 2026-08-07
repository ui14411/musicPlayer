#include "HeaderFile/LoadImage.h"

#include <QDir>
#include <QFileInfo>
#include <QUrl>
#include <QCoreApplication>
#include <QFileSystemWatcher>
#include <iostream>
#include <QMediaMetaData>
#include <QMediaPlayer>

LoadbgImage::LoadbgImage(QObject* parent, QString _path)
	:QAbstractListModel(parent),path(_path)
{
    QFileSystemWatcher* watcher = new QFileSystemWatcher(this);

    watcher->addPath(path);

    connect(watcher, &QFileSystemWatcher::directoryChanged, this, &LoadbgImage::loadImagelist);
}

LoadbgImage::~LoadbgImage()
{
}

void LoadbgImage::loadImagelist()
{
    imagelist.clear();

    beginResetModel();

    QDir dir(path);

    QStringList filters;
    filters << "*.jpg" << "*.jpeg" << "*.png" << "*.bmp" << "*.webp";

    QFileInfoList filelist = dir.entryInfoList(filters, QDir::Files);

    for (const QFileInfo& file : filelist)
    {
        QVariantMap imageMap;

        imageMap["imageName"] = file.completeBaseName();
        imageMap["imagePath"] = file.filePath();

        imagelist.append(imageMap);
    }

    endResetModel();
}

void LoadbgImage::removeImage(int row)
{
    if (row < 0 || row >= imagelist.size())
        return;

    beginRemoveRows(QModelIndex(), row, row);

    QFile::remove(
        imagelist[row]["imagePath"].toString());

    imagelist.removeAt(row);

    endRemoveRows();
}

int LoadbgImage::rowCount(const QModelIndex&) const
{
    return imagelist.size();
}

QVariant LoadbgImage::data(const QModelIndex& index,
    int role) const
{
    if (!index.isValid())
        return {};

    const auto& it = imagelist[index.row()];

    switch (role)
    {
    case ImageNameRole:
        return it["imageName"];

    case ImagePathRole:
        return it["imagePath"];

    default:
        return {};
    }
}

QHash<int, QByteArray> LoadbgImage::roleNames() const
{
    return
    {
        {ImageNameRole,"imageName"},
        {ImagePathRole,"imagePath"}
    };
}