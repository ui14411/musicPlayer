#include "HeaderFile/WebgetCover.h"

#include <QCoreApplication>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileInfo>

WebgetCover::WebgetCover(QObject* parent)
    : QObject(parent)
    , m_coverProcess(new QProcess(this))
    , m_lyricProcess(new QProcess(this))
{
    connect(m_coverProcess, &QProcess::finished, this, &WebgetCover::onCoverProcessFinished);
    connect(m_lyricProcess, &QProcess::finished, this, &WebgetCover::onLyricProcessFinished);
    connect(m_coverProcess, &QProcess::errorOccurred, this, &WebgetCover::onProcessError);
    connect(m_lyricProcess, &QProcess::errorOccurred, this, &WebgetCover::onProcessError);
}

void WebgetCover::searchMusicInfo(const QString& songName, const QString& singer,const float& targetDuration)
{
    // 如果进程还在运行，先终止（避免冲突）
    if (m_coverProcess->state() == QProcess::Running) {
        m_coverProcess->kill();
        m_coverProcess->waitForFinished(100);
    }
    if (m_lyricProcess->state() == QProcess::Running) {
        m_lyricProcess->kill();
        m_lyricProcess->waitForFinished(100);
    }

    startCoverDownload(songName, singer);
    startLyricDownload(songName, singer, targetDuration);
}

void WebgetCover::startCoverDownload(const QString& songName, const QString& singer)
{
    QString scriptPath = QCoreApplication::applicationDirPath() + "/python_exe/" + "MusicCoverFetcher.exe";
    QString saveDir = QCoreApplication::applicationDirPath() + "/cover";

    if (!QFileInfo::exists(scriptPath)) 
    {
        emitError("Cover script not found: " + scriptPath);
        return;
    }

    m_coverProcess->setProgram(scriptPath);
    m_coverProcess->setArguments({songName, singer, saveDir });
    m_coverProcess->start();
}

void WebgetCover::startLyricDownload(const QString& songName, const QString& singer,const float& targetDuration)
{
    QString scriptPath = QCoreApplication::applicationDirPath() + "/python_exe/" + "MusicLyricFetcher.exe";
    QString saveDir = QCoreApplication::applicationDirPath() + "/lrc";

    if (!QFileInfo::exists(scriptPath)) 
    {
        emitError("Lyric script not found: " + scriptPath);
        return;
    }

    m_lyricProcess->setProgram(scriptPath);
    m_lyricProcess->setArguments({songName, singer, saveDir ,QString::number(targetDuration, 'f', 2) });
    m_lyricProcess->start();
}

void WebgetCover::onCoverProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    if (status != QProcess::NormalExit || exitCode != 0) 
    {
        emitError("Cover process crashed or returned error code " + QString::number(exitCode));
        return;
    }

    QByteArray output = m_coverProcess->readAllStandardOutput().trimmed();
    if (output.isEmpty()) 
    {
        emitError("Empty output from cover script");
        return;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(output, &parseError);
    if (parseError.error != QJsonParseError::NoError) 
    {
        emitError("Cover JSON parse error: " + parseError.errorString());
        return;
    }

    QJsonObject obj = doc.object();
    if (obj["status"].toString() == "ok") 
    {
        emit coverReady(obj["path"].toString());
    }
    else 
    {
        emitError("Cover download failed: " + obj["msg"].toString());
    }

    qDebug() << "Python stdout:";
    qDebug().noquote() << output;
}

void WebgetCover::onLyricProcessFinished(
    int exitCode,
    QProcess::ExitStatus status)
{

    QByteArray error =
        m_lyricProcess->readAllStandardError();

    QByteArray output =
        m_lyricProcess->readAllStandardOutput();

    if (status != QProcess::NormalExit || exitCode != 0)
    {
        emitError(
            "Lyric process crashed or returned error code "
            + QString::number(exitCode)
        );
        return;
    }
}
void WebgetCover::onProcessError(QProcess::ProcessError error)
{
    QString msg;
    switch (error) 
    {
    case QProcess::FailedToStart: msg = "Failed to start Python process"; break;
    case QProcess::Crashed: msg = "Process crashed"; break;
    case QProcess::Timedout: msg = "Process timed out"; break;
    default: msg = "Unknown process error"; break;
    }
    emitError(msg);
}

void WebgetCover::emitError(const QString& msg)
{
    qDebug() << "[WebMusicFetcher]" << msg;
    emit errorOccurred(msg);
}