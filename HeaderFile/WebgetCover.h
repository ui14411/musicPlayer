#ifndef WEBGETCOVERR_H
#define WEBGETCOVERR_H

#include <QObject>
#include <QProcess>

class WebgetCover :public QObject
{
	Q_OBJECT;
public:
	WebgetCover(QObject* parent = nullptr);

public:
	Q_INVOKABLE void searchMusicInfo(const QString& musicName, const QString& musicSinger,const float& targetDuration);

signals:
	void coverReady(const QString& filePath);
	void lyricReady(const QString& filePath);
	void errorOccurred(const QString& msg);

private slots:
	void onCoverProcessFinished(int exitCode, QProcess::ExitStatus status);
	void onLyricProcessFinished(int exitCode, QProcess::ExitStatus status);
	void onProcessError(QProcess::ProcessError error);

private:
	QProcess* m_coverProcess;
	QProcess* m_lyricProcess;

private:
	void startCoverDownload(const QString& songName, const QString& singer);
	void startLyricDownload(const QString& songName, const QString& singer, const float& targetDuration);
	void emitError(const QString& msg);
};

#endif