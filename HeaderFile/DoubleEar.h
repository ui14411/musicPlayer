#include <QAbstractListModel>
#include <QVariantMap>
#include <QList>
#include <QThread>

class DoubleEar :public QAbstractListModel
{
	Q_OBJECT
public:
	explicit DoubleEar(QObject* parent = nullptr, QString _path = " ");
	~DoubleEar();

public:
	enum MusicRoles
	{
		MusicNameRole = Qt::UserRole + 1,
		MusicPathRole,
		MusicSingerRole
	};

	Q_INVOKABLE void loadMusiclist();
	Q_INVOKABLE void removeMusic(int row);

	Q_INVOKABLE  QVariantList getAllPaths() const;

public:
	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant data(const QModelIndex& index, int role) const override;
	QHash<int, QByteArray> roleNames() const override;

signals:
	void musiclistChanged();

private:
	QList<QVariantMap> musiclist;

	QString path = "";
	QString musicName = "";
	QString musicSinger = "";

};