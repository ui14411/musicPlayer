#pragma once

#include <vector>
#include <string>
#include <complex>
#include <QString>
#include <atomic>
#include <QFile>

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
struct WebgetCover;

struct HRTFData
{
    int elevation;
    int azimuth;
    QString wavPath;
    std::vector<float>leftIR;
    std::vector<float>rightIR;
};

// 流式 WAV：open → 每块 append → close（close 时回填 RIFF/data 长度）
class WavStreamWriter
{
public:
    bool open(const QString& path, int sampleRate, int channels);
    bool append(const std::vector<float>& pcm);   // float → int16，追加写
    bool close();
private:
    QFile m_file;
    int m_sampleRate = 0;
    int m_channels = 0;
    quint32 m_dataBytes = 0;
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

    // 构建模型输入 tensor (立体声左+右的实部虚部)
    void buildInputTensor(const std::vector<STFTFrame>& leftFrames,
        const std::vector<STFTFrame>& rightFrames,
        int startFrame,
        std::vector<float>& outputTensor);

    // 写 WAV 文件
    bool writeWav(const std::string& filePath,
        const std::vector<float>& pcm,
        int sampleRate, int channels);

    // 分块流式：只算帧区间 [f0,f1) 的 STFT（与整首 stft() 逐帧一致）
    std::vector<STFTFrame> stftRange(const std::vector<float>& pcm,
        int f0, int f1, int fftSize, int hopSize);

    // 分块流式：只重建样本区间 [outBegin,outEnd) 的 ISTFT（与整首 istft() 逐样本一致）
    std::vector<float> istftRange(const std::vector<STFTFrame>& frames,
        int frameOffset, int outBegin, int outEnd, int fftSize, int hopSize);


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

    void getMusicInfo(const QString& path,const QString& name,WebgetCover* web);//获取信息

    //成员变量
private:
    // ONNX Runtime 相关
    void* m_env;        // OrtEnv*
    void* m_session;    // OrtSession*
    void* m_memInfo;    // OrtMemoryInfo*
    std::string m_inputName;
    std::string m_outputName;
    std::string m_lastError;


    // 禁止拷贝
    AudioSeparator(const AudioSeparator&) = delete;
    AudioSeparator& operator=(const AudioSeparator&) = delete;

    //存储wav文件路径
    std::vector<HRTFData> wavFiles;

public:
    std::atomic_bool m_task{ false };

signals:
    void separateProgress(int value);
    void sendtaskName(QString path);
};