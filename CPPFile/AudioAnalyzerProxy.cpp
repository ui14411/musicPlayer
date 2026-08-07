#include "HeaderFile/AudioAnalyzerProxy.h"

#include <QThread>

AudioAnalyzerProxy::AudioAnalyzerProxy(QObject* parent)
    :QObject(parent)
{

    analyzer = new AudioAnalyzer;

    thread = new QThread(this);

    analyzer->moveToThread(thread);

    connect(thread, &QThread::finished, analyzer, &QObject::deleteLater);

    connect(analyzer, &AudioAnalyzer::spectrumChanged, this, &AudioAnalyzerProxy::spectrumChanged, Qt::QueuedConnection);

    thread->start();
}

AudioAnalyzerProxy::~AudioAnalyzerProxy()
{
    if (thread)
    {
        thread->quit();
        thread->wait();
    }
}

void AudioAnalyzerProxy::loadMusic(QString path)
{
    QMetaObject::invokeMethod(
        analyzer,
        "loadMusic",
        Qt::QueuedConnection,
        Q_ARG(QString, path)
    );
}

void AudioAnalyzerProxy::analyzePosition(qint64 pos)
{
    QMetaObject::invokeMethod(
        analyzer,
        "analyzePCM",
        Qt::QueuedConnection,
        Q_ARG(qint64, pos)
    );

}