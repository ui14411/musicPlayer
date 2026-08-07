#include <QAbstractListModel>
#include <QVariantMap>
#include <QList>
#include <QProcess>

class loadlocalVideo :public QAbstractListModel
{
	Q_OBJECT

public:
	explicit loadlocalVideo(QObject* parent = nullptr);

public:
	enum VideoRoles
	{
		VideoNameRoles = Qt::UserRole + 1,
		videoPathRoles
	};

	Q_INVOKABLE void loadVideoList(QString path);

	Q_INVOKABLE void addVideo(const QString& videoPath,const QString& musicName);

	int rowCount(const QModelIndex& parent = QModelIndex()) const override;

	QVariant data(const QModelIndex& index, int role) const override;

	QHash<int, QByteArray> roleNames() const override;

signals:
	void videolistChanged();
	void addVideoFailed(QString msg);

private:
	QList<QVariantMap> videolist;

	QProcess* ffmpeg = nullptr;

	QString pendingPath;
	QString videoName;
};