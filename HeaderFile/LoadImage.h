#include <QAbstractListModel>
#include <QVariantMap>
#include <QList>

class LoadbgImage :public QAbstractListModel
{
	Q_OBJECT
public:
	explicit LoadbgImage(QObject* parent = nullptr, QString _path = " ");
	~LoadbgImage();

public:
	enum ImageRoles
	{
		ImageNameRole = Qt::UserRole + 1,
		ImagePathRole,
	};

	Q_INVOKABLE void loadImagelist();
	Q_INVOKABLE void removeImage(int row);

public:
	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant data(const QModelIndex& index, int role) const override;
	QHash<int, QByteArray> roleNames() const override;

signals:
	void imagelistChanged();

private:
	QList<QVariantMap> imagelist;

	QString path = "";
	QString imageName = "";
};