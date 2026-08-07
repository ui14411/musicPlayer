#pragma once

#include <vector>
#include <string>
#include <memory>
#include <complex>
#include <QString>
#include <QAudioSink>
#include <QAudioFormat>
#include <QMediaDevices>
#include <QThread>
#include <atomic>

extern "C" 
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <kiss_fft.h>
}

struct OrtSession;
struct OrtEnv;
struct OrtMemoryInfo;
struct OrtSessionOptions;

struct HRTFData
{
    int elevation;
    int azimuth;
    QString wavPath;
    std::vector<float>leftIR;
    std::vector<float>rightIR;
};

struct WavHeader
{
    char riff[4];
    uint32_t fileSize;
    char wave[4];

    char fmt[4];
    uint32_t fmtSize;

    uint16_t audioFormat;
    uint16_t channels;

    uint32_t sampleRate;
    uint32_t byteRate;

    uint16_t blockAlign;
    uint16_t bitsPerSample;

    char dataTag[4];
    uint32_t dataSize;
};

class AudioSeparator : public QObject
{
    Q_OBJECT
public:
    AudioSeparator(QObject* parent = nullptr);
    ~AudioSeparator();
public slots:
    // 加载 ONNX 模型，modelPath 为 UTF-8 路径
    bool loadModel(const std::string& modelPath,
        const std::string& inputName = "input",
        const std::string& outputName = "output");

    // 分离音频文件，输入路径，输出路径
    bool separate(const std::string& inputFile);

    // 获取最后一次错误的描述
    std::string getLastError() const { return m_lastError; }

    //环绕音
    bool Surrounding(const QString& filePath);

    //双耳分听
    bool doubleEarListening(const QString& leftPath, const QString& rightPath,
        const QString& Lname, const QString& Rname);

    void cancelTask();//任务取消机制

//成员函数
private:
    // 解码为立体声 PCM (float, 44100Hz)
    bool decodeAudio(const std::string& filePath,
        std::vector<float>& stereoPcm,
        int& sampleRate);

    // STFT (单声道)
    struct STFTFrame 
    {
        std::vector<std::complex<float>> spectrum; // 长度 = fftSize/2+1
    };

    std::vector<STFTFrame> stft(const std::vector<float>& pcm, int fftSize, int hopSize);

    // ISTFT (单声道)
    std::vector<float> istft(const std::vector<STFTFrame>& frames, int fftSize, int hopSize);

    // 构建模型输入 tensor (立体声左+右的实部虚部)
    void buildInputTensor(const std::vector<STFTFrame>& leftFrames,
        const std::vector<STFTFrame>& rightFrames,
        int startFrame,
        std::vector<float>& outputTensor);

    // 写 WAV 文件
    bool writeWav(const std::string& filePath,
        const std::vector<float>& pcm,
        int sampleRate, int channels);

    //读取HRTR文件，读入内存
    bool decodeHRTF();

    //查找最近的两个HRIR
    bool FindTwoNearestHRIR(
        int elevation,
        float azimuth,
        HRTFData*& h1,
        HRTFData*& h2,
        float& alpha);

    void Convolve(const std::vector<float>& input, const std::vector<float>& ir, std::vector<float>& output);

    bool getInterpolatedIR(
        float elevation,
        float azimuth,
        std::vector<float>& leftIR,
        std::vector<float>& rightIR
    );

    void ProcessVirtualSource(const std::vector<float>& inputBlock,
        float elevation, float azimuth, float midSideGain, 
        size_t pos, std::vector<float>& Lout, std::vector<float>& Rout, 
        std::vector<float>& Ldry, std::vector<float>& Rdry, int blockSize);

    //成员变量
private:
    // ONNX Runtime 相关
    void* m_env;        // OrtEnv*
    void* m_session;    // OrtSession*
    void* m_memInfo;    // OrtMemoryInfo*
    std::string m_inputName;
    std::string m_outputName;
    std::string m_lastError;

    QString musicName = "";

    bool LoadIR(int elevation, float azimuth, std::vector<float>& leftIR, std::vector<float>& rightIR);

    // 禁止拷贝
    AudioSeparator(const AudioSeparator&) = delete;
    AudioSeparator& operator=(const AudioSeparator&) = delete;

    //存储wav文件路径
    std::vector<HRTFData> wavFiles;

    std::atomic_bool m_task{ false };

    struct whisper_context* m_whisperCtx = nullptr;

signals:
    void separateProgress(int value);
    void sendtaskName(QString path);
    void separatefished();
    void surroundfished();
};