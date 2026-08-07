#pragma once

#include <QObject>
#include <QVariantList>
#include <QThread>

#include "HeaderFile/AudioAnalyzer.h"

class AudioAnalyzerProxy :public QObject
{
	Q_OBJECT
public:

    explicit AudioAnalyzerProxy(QObject* parent = nullptr);
    ~AudioAnalyzerProxy();
public:
    Q_INVOKABLE void loadMusic(QString path);

    void analyzePosition(qint64 pos);

signals:
    void spectrumChanged(QVariantList data);

private:
    QThread* thread;
    AudioAnalyzer* analyzer;
};