#include <QAbstractListModel>
#include <QVariantMap>
#include <QList>
#include <QThread>
#include <QCoreApplication>

#include "HeaderFile/AudioSeparator.h"
#include "HeaderFile/WebgetCover.h"

class loadlocalMusic :public QAbstractListModel
{
	Q_OBJECT

	Q_PROPERTY(int value
	READ getProcess
	NOTIFY processChanged)

	Q_PROPERTY(QString taskName
		READ getTaskname
		NOTIFY taskNameChanged)

public:
	explicit loadlocalMusic(QObject* parent = nullptr,QString _path = " ");
	~loadlocalMusic();

public:
	enum MusicRoles
	{
		MusicNameRole = Qt::UserRole + 1,
		MusicPathRole,
		MusicSingerRole,
		MusicCoverRole
	};
	
	Q_INVOKABLE void loadMusiclist();
	Q_INVOKABLE void removeMusic(int row);
	Q_INVOKABLE void removeVideo(int row);
	Q_INVOKABLE void removelrc(int row);
	Q_INVOKABLE void addMusic(const QString& musicPath);
	Q_INVOKABLE void addMusicLrc(const QString& musicLrcPath, const QString& musicName);
	Q_INVOKABLE void addMusicCover(const QString& musicLrcPath, const QString& musicName);
	Q_INVOKABLE void playDimensionalMusic(const QString& l, const QString& r);
	Q_INVOKABLE void replaceModel(const QString& filePath);

public:
	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant data(const QModelIndex& index, int role) const override;
	QHash<int, QByteArray> roleNames() const override;
	QString getTaskname() { return m_taskName; };
	int getProcess() { return m_process; };
	void getMusicinfo_addmusic(int row, const QString& path);

	void getMusicinfo_loadmusicList(int row, const QString& path);

	QString findFile(const QString& folder, const QString& baseName);

	void setOnnxPath();

private slots: 
	void setProcess(int value);
	void setTaskname(QString path);

signals:
	void musiclistChanged();
	void startSurrounding(const QString& path);
	void startOnnx(const std::string& path);
	void processChanged();
	void taskNameChanged();
	void startDouble(const QString& leftPath, const QString& rightPath,
		const QString& Lname, const QString& Rnam);
	void musicInfoReady(QString name, QString singer);
	void musicCoverChanged(QString musicPath);
	void musicLrcChanged(QString musicPath);

private:
	QList<QVariantMap> musiclist;

	QString path = "";
	QString musicName = "";
	QString musicSinger = "";

	AudioSeparator* as = nullptr;
	AudioSeparator* asSurrounding = nullptr;
	QThread* thread1 = nullptr;
	QThread* thread2 = nullptr;

	QString m_taskName = "";
	int m_process = 0;

	QString onnxPath = "";

	WebgetCover* webgetCover = nullptr;
};