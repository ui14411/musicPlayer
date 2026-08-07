#ifndef AUDIOANALYZER_H
#define AUDIOANALYZER_H

#include <QObject>
#include <vector>
#include <complex>
#include <QString>
#include <QVariant>

class AudioAnalyzer : public QObject
{
    Q_OBJECT

public:
    explicit AudioAnalyzer(QObject* parent = nullptr);

    //获取最后的错误信息
    QString getLastError() const { return m_lastError; };

public slots:
    //加载音乐
    bool loadMusic(QString path);

    void analyzePCM(qint64 position);

signals:
    //发送频谱给qml
    void spectrumChanged(QVariantList data);

private:
    //音乐解码
    bool decodeAudio(const std::string& filePath, std::vector<float>& stereoPcm, int& sampleRate);

private:

    QString m_musicPath;

    QString m_lastError;

    std::vector<float> m_pcm;

    int m_sampleRate = 44100;
};

#endif