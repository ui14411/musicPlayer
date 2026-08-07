#pragma once

#include <QObject>

class PlayMusic;
class loadlocalMusic;
class DoubleEar;
class LoadbgImage;
class PlayDouble;
class AudioAnalyzerProxy;
class loadlocalVideo;


class InitManage :public QObject {

	Q_OBJECT

	Q_PROPERTY(int progress
		READ getProgress
		NOTIFY progressChanged)

public:
	InitManage(QObject* parent = nullptr,
		PlayMusic* player = nullptr,
		loadlocalMusic* musicload = nullptr,
		DoubleEar* Dmusic = nullptr,
		LoadbgImage* image = nullptr,
		PlayDouble* doublePlayer = nullptr,
		AudioAnalyzerProxy* analyzer = nullptr,
		loadlocalVideo* videoload = nullptr);
public:
	Q_INVOKABLE void start();

public:
    int getProgress() const { return m_progress; };

signals:
	void progressChanged();
	void finished();

private:
    int m_progress = 0;

	PlayMusic* player;
	loadlocalMusic* musicload;
	DoubleEar* Dmusic;
	LoadbgImage* image;
	PlayDouble* doublePlayer;
	AudioAnalyzerProxy* analyzer;
	loadlocalVideo* videoload;
};