#include "HeaderFile/AudioSeparator.h"
#include "HeaderFile/LoadlocalMusic.h"
#include "HeaderFile/WebgetCover.h"

#include <onnxruntime_cxx_api.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include "kiss_fft.h"

#include <QTimer>
#include <cmath>
#include <fstream>
#include <algorithm>
#include <thread>
#include <cstring>
#include <iostream>
#include <QFile>
#include <QFileInfo>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfoList>
#include <QRegularExpression>
#include <QDebug>
#include <set>
#include <QAudioDevice>
#include <QMediaMetaData>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <QMediaPlayer>

AudioSeparator::AudioSeparator(QObject* parent) 
    :QObject(parent),m_env(nullptr), m_session(nullptr), m_memInfo(nullptr) 
{
}

AudioSeparator::~AudioSeparator() 
{
    if (m_session) delete static_cast<Ort::Session*>(m_session);
    if (m_memInfo) delete static_cast<Ort::MemoryInfo*>(m_memInfo);
    if (m_env) delete static_cast<Ort::Env*>(m_env);
}

bool AudioSeparator::loadModel(const std::string& modelPath,
    const std::string& inputName,
    const std::string& outputName)
{
    try 
    {
        m_env = new Ort::Env(ORT_LOGGING_LEVEL_WARNING, "AudioSeparator");
        Ort::SessionOptions opts;
        int numThreads = std::min(8, (int)std::thread::hardware_concurrency());
        opts.SetIntraOpNumThreads(numThreads);
        opts.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        opts.EnableCpuMemArena();
        opts.EnableMemPattern();

        std::wstring wPath(modelPath.begin(), modelPath.end());
        m_session = new Ort::Session(*static_cast<Ort::Env*>(m_env), wPath.c_str(), opts);

        m_inputName = inputName;
        m_outputName = outputName;
        m_memInfo = new Ort::MemoryInfo(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault));
        return true;
    }
    catch (const Ort::Exception& e)
    {
        m_lastError = "ONNX Runtime error: " + std::string(e.what());
        return false;
    }
}

bool AudioSeparator::decodeAudio(const std::string& filePath,
    std::vector<float>& stereoPcm,
    int& sampleRate) 
{
    stereoPcm.clear();
    AVFormatContext* fmtCtx = nullptr;
    if (avformat_open_input(&fmtCtx, filePath.c_str(), nullptr, nullptr) < 0)
    {
        m_lastError = "Failed to open file";
        qDebug() << m_lastError;
        return false;
    }
    if (avformat_find_stream_info(fmtCtx, nullptr) < 0)
    {
        avformat_close_input(&fmtCtx);
        m_lastError = "Failed to find stream info";
        qDebug() << m_lastError;
        return false;
    }

    int audioIdx = -1;
    for (unsigned i = 0; i < fmtCtx->nb_streams; ++i)
    {
        if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            audioIdx = i;
            break;
        }
    }
    if (audioIdx == -1) 
    {
        avformat_close_input(&fmtCtx);
        m_lastError = "No audio stream";
        qDebug() << m_lastError;
        return false;
    }

    AVCodecParameters* codecPar = fmtCtx->streams[audioIdx]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecPar->codec_id);
    if (!codec)
    {
        avformat_close_input(&fmtCtx);
        m_lastError = "Decoder not found";
        qDebug() << m_lastError;
        return false;
    }

    AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx, codecPar);
    if (avcodec_open2(codecCtx, codec, nullptr) < 0)
    {
        avcodec_free_context(&codecCtx);
        avformat_close_input(&fmtCtx);
        m_lastError = "Failed to open codec";
        qDebug() << m_lastError;
        return false;
    }

    sampleRate = 44100;
    int targetChannels = 2;
    AVChannelLayout targetLayout = AV_CHANNEL_LAYOUT_STEREO;
    SwrContext* swr = nullptr;
    swr_alloc_set_opts2(&swr, &targetLayout, AV_SAMPLE_FMT_FLT, sampleRate,
        &codecCtx->ch_layout, codecCtx->sample_fmt, codecCtx->sample_rate, 0, nullptr);
    if (!swr || swr_init(swr) < 0)
    {
        avcodec_free_context(&codecCtx);
        avformat_close_input(&fmtCtx);
        m_lastError = "Resampler init failed";
        qDebug() << m_lastError;
        return false;
    }

    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();

    while (av_read_frame(fmtCtx, pkt) >= 0) 
    {
        if (pkt->stream_index != audioIdx) 
        {
            av_packet_unref(pkt);
            continue;
        }
        if (avcodec_send_packet(codecCtx, pkt) < 0)
        {
            av_packet_unref(pkt);
            continue;
        }
        while (avcodec_receive_frame(codecCtx, frame) == 0) 
        {
            int outSamples = av_rescale_rnd(swr_get_delay(swr, codecCtx->sample_rate) + frame->nb_samples,
                sampleRate, codecCtx->sample_rate, AV_ROUND_UP);
            std::vector<float> buffer(outSamples * targetChannels);
            uint8_t* out[] = { reinterpret_cast<uint8_t*>(buffer.data()) };
            int converted = swr_convert(swr, out, outSamples, (const uint8_t**)frame->data, frame->nb_samples);
            if (converted > 0) 
            {
                stereoPcm.insert(stereoPcm.end(), buffer.begin(), buffer.begin() + converted * targetChannels);
            }
        }
        av_packet_unref(pkt);
    }
    // flush decoder
    avcodec_send_packet(codecCtx, nullptr);
    while (avcodec_receive_frame(codecCtx, frame) == 0) 
    {
        int outSamples = av_rescale_rnd(swr_get_delay(swr, codecCtx->sample_rate) + frame->nb_samples,
            sampleRate, codecCtx->sample_rate, AV_ROUND_UP);
        std::vector<float> buffer(outSamples * targetChannels);
        uint8_t* out[] = { reinterpret_cast<uint8_t*>(buffer.data()) };
        int converted = swr_convert(swr, out, outSamples, (const uint8_t**)frame->data, frame->nb_samples);
        if (converted > 0)
            stereoPcm.insert(stereoPcm.end(), buffer.begin(), buffer.begin() + converted * targetChannels);
    }
    // flush resampler
    uint8_t* dummy[] = { nullptr };
    int remaining = swr_convert(swr, dummy, 0, nullptr, 0);
    if (remaining > 0) 
    {
        std::vector<float> buffer(remaining * targetChannels);
        uint8_t* out[] = { reinterpret_cast<uint8_t*>(buffer.data()) };
        swr_convert(swr, out, remaining, nullptr, 0);
        stereoPcm.insert(stereoPcm.end(), buffer.begin(), buffer.end());
    }

    av_frame_free(&frame);
    av_packet_free(&pkt);
    swr_free(&swr);
    avcodec_free_context(&codecCtx);
    avformat_close_input(&fmtCtx);
    return true;
}

void AudioSeparator::buildInputTensor(const std::vector<STFTFrame>& leftFrames,
    const std::vector<STFTFrame>& rightFrames,
    int startFrame,
    std::vector<float>& outputTensor) 
{
    const int numChannels = 4;
    const int targetBins = 3072;
    const int patchFrames = 256;
    const int srcBins = (int)leftFrames[0].spectrum.size(); // 3073
    outputTensor.assign(numChannels * targetBins * patchFrames, 0.0f);

    for (int t = 0; t < patchFrames; ++t) 
    {
        int frameIdx = startFrame + t;
        if (frameIdx >= (int)leftFrames.size()) break;
        const auto& leftSpec = leftFrames[frameIdx].spectrum;
        const auto& rightSpec = rightFrames[frameIdx].spectrum;

        for (int f = 0; f < targetBins; ++f) 
        {
            int idx = f * patchFrames + t;
            outputTensor[0 * targetBins * patchFrames + idx] = leftSpec[f].real();
            outputTensor[1 * targetBins * patchFrames + idx] = leftSpec[f].imag();
            outputTensor[2 * targetBins * patchFrames + idx] = rightSpec[f].real();
            outputTensor[3 * targetBins * patchFrames + idx] = rightSpec[f].imag();
        }
    }
}

bool AudioSeparator::writeWav(
    const std::string& filePath,
    const std::vector<float>& pcm,
    int sampleRate,
    int channels)
{
    QFile file(QString::fromStdString(filePath));

    if (!file.open(QIODevice::WriteOnly))
    {
        m_lastError =
            "file open failed: " +
            QString::fromStdString(filePath).toStdString();

        return false;
    }

    int numSamples = static_cast<int>(pcm.size());
    int dataSize = numSamples * sizeof(int16_t);

    // float -> int16
    std::vector<int16_t> intData(numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        int sample = static_cast<int>(pcm[i] * 32767.0f);

        sample = std::clamp(sample, -32768, 32767);

        intData[i] = static_cast<int16_t>(sample);
    }

    // WAV Header
    file.write("RIFF", 4);

    int chunkSize = 36 + dataSize;
    file.write(reinterpret_cast<const char*>(&chunkSize), 4);

    file.write("WAVE", 4);

    // fmt chunk
    file.write("fmt ", 4);

    int subchunk1Size = 16;
    file.write(reinterpret_cast<const char*>(&subchunk1Size), 4);

    short audioFormat = 1; // PCM
    file.write(reinterpret_cast<const char*>(&audioFormat), 2);

    short numChannels = static_cast<short>(channels);
    file.write(reinterpret_cast<const char*>(&numChannels), 2);

    file.write(reinterpret_cast<const char*>(&sampleRate), 4);

    int byteRate = sampleRate * channels * 2;
    file.write(reinterpret_cast<const char*>(&byteRate), 4);

    short blockAlign = static_cast<short>(channels * 2);
    file.write(reinterpret_cast<const char*>(&blockAlign), 2);

    short bitsPerSample = 16;
    file.write(reinterpret_cast<const char*>(&bitsPerSample), 2);

    // data chunk
    file.write("data", 4);
    file.write(reinterpret_cast<const char*>(&dataSize), 4);

    // PCM 数据
    file.write(
        reinterpret_cast<const char*>(intData.data()),
        dataSize
    );

    file.close();

    return true;
}

//  流式 WAV 写入,close 时回填头长度
bool WavStreamWriter::open(const QString& path, int sampleRate, int channels)
{
    m_sampleRate = sampleRate;
    m_channels = channels;
    m_dataBytes = 0;
    m_file.setFileName(path);
    if (!m_file.open(QIODevice::WriteOnly))
        return false;
    m_file.write(QByteArray(44, '\0'));   // 先占位 44 字节头
    return true;
}

bool WavStreamWriter::append(const std::vector<float>& pcm)
{
    if (!m_file.isOpen())
        return false;
    std::vector<int16_t> d(pcm.size());
    for (size_t i = 0; i < pcm.size(); ++i)
    {
        int s = static_cast<int>(pcm[i] * 32767.0f);
        s = std::clamp(s, -32768, 32767);
        d[i] = static_cast<int16_t>(s);
    }
    m_file.write(reinterpret_cast<const char*>(d.data()), (qint64)d.size() * 2);
    m_dataBytes += static_cast<quint32>(d.size() * 2);
    return true;
}

bool WavStreamWriter::close()
{
    if (!m_file.isOpen())
        return false;

    m_file.seek(0);
    const int dataSize = static_cast<int>(m_dataBytes);

    m_file.write("RIFF", 4);
    int chunkSize = 36 + dataSize;
    m_file.write(reinterpret_cast<const char*>(&chunkSize), 4);
    m_file.write("WAVE", 4);

    m_file.write("fmt ", 4);
    int subchunk1Size = 16;
    m_file.write(reinterpret_cast<const char*>(&subchunk1Size), 4);

    short audioFormat = 1;   // PCM
    m_file.write(reinterpret_cast<const char*>(&audioFormat), 2);

    short numChannels = static_cast<short>(m_channels);
    m_file.write(reinterpret_cast<const char*>(&numChannels), 2);

    int sr = m_sampleRate;
    m_file.write(reinterpret_cast<const char*>(&sr), 4);

    int byteRate = m_sampleRate * m_channels * 2;
    m_file.write(reinterpret_cast<const char*>(&byteRate), 4);

    short blockAlign = static_cast<short>(m_channels * 2);
    m_file.write(reinterpret_cast<const char*>(&blockAlign), 2);

    short bitsPerSample = 16;
    m_file.write(reinterpret_cast<const char*>(&bitsPerSample), 2);

    m_file.write("data", 4);
    m_file.write(reinterpret_cast<const char*>(&dataSize), 4);

    m_file.close();
    return true;
}

// 分块流式：只算帧区间 [f0,f1) 的 STFT
std::vector<AudioSeparator::STFTFrame> AudioSeparator::stftRange(
    const std::vector<float>& pcm, int f0, int f1, int fftSize, int hopSize)
{
    std::vector<STFTFrame> frames;
    if (f1 <= f0) return frames;

    kiss_fft_cfg cfg = kiss_fft_alloc(fftSize, 0, nullptr, nullptr);
    if (!cfg) return frames;

    std::vector<float> window(fftSize);
    for (int i = 0; i < fftSize; ++i)
        window[i] = 0.54f - 0.46f * cos(2.0 * M_PI * i / (fftSize - 1));

    int specSize = fftSize / 2 + 1;
    std::vector<kiss_fft_cpx> in(fftSize);
    std::vector<kiss_fft_cpx> out(fftSize);

    frames.reserve((size_t)(f1 - f0));
    for (int fi = f0; fi < f1; ++fi)
    {
        size_t start = (size_t)fi * hopSize;
        if (start + fftSize > pcm.size()) break;
        for (int j = 0; j < fftSize; ++j)
        {
            in[j].r = pcm[start + j] * window[j];
            in[j].i = 0.0f;
        }
        kiss_fft(cfg, in.data(), out.data());
        STFTFrame frame;
        frame.spectrum.resize(specSize);
        for (int k = 0; k < specSize; ++k)
            frame.spectrum[k] = std::complex<float>(out[k].r, out[k].i);
        frames.push_back(std::move(frame));
    }
    free(cfg);
    return frames;
}

// 分块流式：只重建样本区间 [outBegin,outEnd) 的 ISTFT
// frames[0] 对应全局帧号 frameOffset；输出与整首一次算完逐样本一致。
std::vector<float> AudioSeparator::istftRange(
    const std::vector<STFTFrame>& frames, int frameOffset,
    int outBegin, int outEnd, int fftSize, int hopSize)
{
    std::vector<float> result;
    if (outEnd <= outBegin || frames.empty()) return result;

    kiss_fft_cfg cfg = kiss_fft_alloc(fftSize, 1, nullptr, nullptr);
    if (!cfg) return result;

    const int win = outEnd - outBegin;
    std::vector<float> output(win, 0.0f);
    std::vector<float> weight(win, 0.0f);

    std::vector<float> window(fftSize);
    for (int i = 0; i < fftSize; ++i)
        window[i] = 0.54f - 0.46f * cos(2.0 * M_PI * i / (fftSize - 1));

    int half = fftSize / 2;
    std::vector<kiss_fft_cpx> in(fftSize);
    std::vector<kiss_fft_cpx> out(fftSize);

    for (size_t fi = 0; fi < frames.size(); ++fi)
    {
        const auto& spec = frames[fi].spectrum;
        in[0].r = spec[0].real();
        in[0].i = 0.0f;
        for (int k = 1; k < half; ++k)
        {
            in[k].r = spec[k].real();
            in[k].i = spec[k].imag();
        }
        if (fftSize % 2 == 0)
        {
            in[half].r = spec[half].real();
            in[half].i = 0.0f;
        }
        for (int k = half + 1; k < fftSize; ++k)
        {
            int c = fftSize - k;
            in[k].r = in[c].r;
            in[k].i = -in[c].i;
        }
        kiss_fft(cfg, in.data(), out.data());

        const int start = (frameOffset + (int)fi) * hopSize;
        for (int n = 0; n < fftSize; ++n)
        {
            const int p = start + n;
            if (p < outBegin || p >= outEnd) continue;
            float s = out[n].r / fftSize;
            s *= window[n];
            output[p - outBegin] += s;
            weight[p - outBegin] += window[n] * window[n];
        }
    }
    for (int i = 0; i < win; ++i)
    {
        if (weight[i] > 1e-6f)
            output[i] /= weight[i];
    }
    free(cfg);
    return output;
}

bool AudioSeparator::decodeHRTF()
{
    wavFiles.clear();

    QString path = QCoreApplication::applicationDirPath() + "/HRTF";

    QDir dir(path);

    if (!dir.exists())
    {
        m_lastError = "[ERROR decodeHRTF] path is error";
        return false;
    }

    QFileInfoList files =
        dir.entryInfoList(
            QStringList() << "*.wav",
            QDir::Files);

    QRegularExpression reg(
        R"(IRC_1002_C_R0195_T(\d+)_P(\d+)\.wav)");

    for (const QFileInfo& file : files)
    {
        QString name = file.fileName();

        auto match = reg.match(name);

        if (!match.hasMatch())
            continue;

        HRTFData data;

        data.elevation =
            match.captured(1).toInt();

        data.azimuth =
            match.captured(2).toInt();

        data.wavPath =
            file.absoluteFilePath();

        // 读取 wav
        std::vector<float> pcm;

        int sr = 0;

        if (!decodeAudio(
            data.wavPath.toStdString(),
            pcm,
            sr))
        {
            qDebug() << "Load Failed:" << data.wavPath;

            continue;
        }

        // 拆左右耳
        data.leftIR.reserve(pcm.size() / 2);
        data.rightIR.reserve(pcm.size() / 2);

        for (int i = 0; i < pcm.size(); i += 2)
        {
            data.leftIR.push_back(pcm[i]);
            data.rightIR.push_back(pcm[i + 1]);
        }

        wavFiles.push_back(std::move(data));
    }

    qDebug() << "Load HRIR Count:" << wavFiles.size();

    return !wavFiles.empty();
}

bool AudioSeparator::Surrounding(const QString& filePath)
{
    m_task.store(false);
    QElapsedTimer timer;
    timer.start();
    QFile file(filePath);
    QFileInfo fileinfo(filePath);

    if (wavFiles.empty())
        decodeHRTF();

    if (!file.exists())
    {
        m_lastError = "[ERROR Surrounding]file is open fail";
        return false;
    }

    if (m_task) return false;

    std::vector<float> stereoPcm;
    int sampleRate = 0;
    if (!decodeAudio(filePath.toStdString(), stereoPcm, sampleRate))
        return false;

    if (m_task) return false;

    QString outpath = QCoreApplication::applicationDirPath() + "/surrounding/" + fileinfo.fileName();
    QDir().mkpath(QCoreApplication::applicationDirPath() + "/surrounding");

    //1. 分离立体声
    std::vector<float> leftPcm, rightPcm;
    for (size_t i = 0; i < stereoPcm.size(); i += 2)
    {
        leftPcm.push_back(stereoPcm[i]);
        rightPcm.push_back(stereoPcm[i + 1]);
    }
    if (m_task) return false;
    //2. 收集可用的整数仰角
    std::set<int> elevSet;
    for (const auto& h : wavFiles)
        elevSet.insert(h.elevation);
    std::vector<int> availableElevations(elevSet.begin(), elevSet.end());
    if (availableElevations.empty())
        return false;
    if (m_task) return false;
    //3. 参数
    const int blockSize = sampleRate / 60;            // ~17 ms
    const size_t totalSamples = std::min(leftPcm.size(), rightPcm.size());
    const float rotSpeed = 45.0f;                      // rotSpeed/360 s
    const float stereoOffset = 45.0f;                      // 左右声道张开
    const float elevAmp = 0.0f;                      // 仰角浮动幅度
    const float elevFreq = 0.0f;                       // 仰角变化频率
    const int   delaySamps = static_cast<int>(sampleRate * 0.0005f); // 0.05ms
    const float delayGain = 0.2f;
    const float midSideGain = 2.0f;
    if (m_task) return false;
    //输出缓冲区
    std::vector<float> Lout(totalSamples + 4096, 0.0f);
    std::vector<float> Rout(totalSamples + 4096, 0.0f);
    std::vector<float> Ldry(totalSamples + 4096, 0.0f);
    std::vector<float> Rdry(totalSamples + 4096, 0.0f);

    std::vector<float> Lout1(totalSamples + 4096, 0.0f);
    std::vector<float> Rout1(totalSamples + 4096, 0.0f);
    std::vector<float> Ldry1(totalSamples + 4096, 0.0f);
    std::vector<float> Rdry1(totalSamples + 4096, 0.0f);

    std::vector<float> Lout2(totalSamples + 4096, 0.0f);
    std::vector<float> Rout2(totalSamples + 4096, 0.0f);
    std::vector<float> Ldry2(totalSamples + 4096, 0.0f);
    std::vector<float> Rdry2(totalSamples + 4096, 0.0f);

    std::vector<float>leftPart;
    std::vector<float>rightPart;
    if (m_task) return false;
    //4.主处理循环 
    for (size_t pos = 0; pos < totalSamples; pos += blockSize)
    {
        if (m_task) return false;
        size_t end = std::min(pos + blockSize, totalSamples);
        std::vector<float> blkL(leftPcm.begin() + pos, leftPcm.begin() + end);
        std::vector<float> blkR(rightPcm.begin() + pos, rightPcm.begin() + end);

        float t = static_cast<float>(pos) / sampleRate;

        // 动态方位角
        float angle = std::fmod(t * rotSpeed, 360.0f);
        if (angle < 0.0f) angle += 360.0f;

        // 动态仰角
        float elevF = elevAmp * std::sin(t * 2.0f * M_PI * elevFreq);

        // 左右源方位角
        float azL = std::fmod(angle + stereoOffset, 360.0f);
        float azR = std::fmod(angle - stereoOffset, 360.0f);
        if (azL < 0.0f) azL += 360.0f;
        if (azR < 0.0f) azR += 360.0f;

        auto leftTask = std::async(std::launch::async, &AudioSeparator::ProcessVirtualSource, this, std::cref(blkL), elevF, azL, midSideGain, pos, std::ref(Lout1), std::ref(Rout1), std::ref(Ldry1), std::ref(Rdry1), blockSize);

        auto rightTask = std::async(std::launch::async, &AudioSeparator::ProcessVirtualSource, this, std::cref(blkR), elevF, azR, midSideGain, pos, std::ref(Lout2), std::ref(Rout2), std::ref(Ldry2), std::ref(Rdry2), blockSize);

        leftTask.get();
        rightTask.get();
        if (m_task) return false;
    }
    if (m_task) return false;
    for (size_t i = 0; i < totalSamples + 4096; ++i)
    {
        Lout[i] += Lout1[i] + Lout2[i];
        Rout[i] += Rout1[i] + Rout2[i];

        Ldry[i] += Ldry1[i] + Ldry2[i];
        Rdry[i] += Rdry1[i] + Rdry2[i];
    }
    if (m_task) return false;
    // 5. 对侧延迟 + 动态 Pan 
    for (size_t i = 0; i < totalSamples; ++i)
    {
        float t = static_cast<float>(i) / sampleRate;
        float angle = std::fmod(t * rotSpeed, 360.0f);
        if (angle < 0.0f) angle += 360.0f;
        float pan = std::sin(angle * M_PI / 180.0f);

        if (pan > 0.05f)
        {
            int idx = static_cast<int>(i) - delaySamps;
            if (idx >= 0) Lout[i] += Ldry[idx] * delayGain;
        }
        else if (pan < -0.05f)
        {
            int idx = static_cast<int>(i) - delaySamps;
            if (idx >= 0) Rout[i] += Rdry[idx] * delayGain;
        }

        float gainL = std::sqrt(0.5f * (1.0f - pan));
        float gainR = std::sqrt(0.5f * (1.0f + pan));
        Lout[i] *= gainL;
        Rout[i] *= gainR;
    }
    if (m_task) return false;
    // 5.5 交叉早期反射（增强左右环绕深度
    const int xDelay1 = static_cast<int>(sampleRate * 0.008f);   // 8 ms
    const int xDelay2 = static_cast<int>(sampleRate * 0.018f);   // 18 ms
    const int xDelay3 = static_cast<int>(sampleRate * 0.031f);   // 31 ms
    const float xGain1 = 0.22f;
    const float xGain2 = 0.3f;
    const float xGain3 = 0.18f;
    const float xLPF1 = 0.1f;   // 低通系数（0~1，越小越亮）
    const float xLPF2 = 0.3f;
    const float xLPF3 = 0.5f;

    // 需要为每个延迟线保存低通状态（因为要交叉，所以左右各一组）
    float xLpL1 = 0.0f, xLpR1 = 0.0f;
    float xLpL2 = 0.0f, xLpR2 = 0.0f;
    float xLpL3 = 0.0f, xLpR3 = 0.0f;
    if (m_task) return false;
    for (size_t i = xDelay3; i < totalSamples; ++i)
    {
        if (m_task) return false;
        // 左声道反射：取左声道旧信号，送入右声道
        float refL1 = Lout[i - xDelay1] * xGain1;
        xLpL1 = xLPF1 * refL1 + (1.0f - xLPF1) * xLpL1;
        Rout[i] += xLpL1;

        float refL2 = Lout[i - xDelay2] * xGain2;
        xLpL2 = xLPF2 * refL2 + (1.0f - xLPF2) * xLpL2;
        Rout[i] += xLpL2;

        float refL3 = Lout[i - xDelay3] * xGain3;
        xLpL3 = xLPF3 * refL3 + (1.0f - xLPF3) * xLpL3;
        Rout[i] += xLpL3;

        // 右声道反射：取右声道旧信号，送入左声道
        float refR1 = Rout[i - xDelay1] * xGain1;
        xLpR1 = xLPF1 * refR1 + (1.0f - xLPF1) * xLpR1;
        Lout[i] += xLpR1;

        float refR2 = Rout[i - xDelay2] * xGain2;
        xLpR2 = xLPF2 * refR2 + (1.0f - xLPF2) * xLpR2;
        Lout[i] += xLpR2;

        float refR3 = Rout[i - xDelay3] * xGain3;
        xLpR3 = xLPF3 * refR3 + (1.0f - xLPF3) * xLpR3;
        Lout[i] += xLpR3;
        if (m_task) return false;
    }

    // 6.小房间混响
    const int d1 = static_cast<int>(sampleRate * 0.13f);
    const int d2 = static_cast<int>(sampleRate * 0.28f);
    const int d3 = static_cast<int>(sampleRate * 0.45f);
    const float g1 = 0.22f, g2 = 0.15f, g3 = 0.08f;
    const float lpf = 0.3f;
    float lpL = 0.0f, lpR = 0.0f;

    if (m_task) return false;

    for (size_t i = d3; i < Lout.size(); ++i)
    {
        float refL = Lout[i - d1] * g1 + Lout[i - d2] * g2 + Lout[i - d3] * g3;
        float refR = Rout[i - d1] * g1 + Rout[i - d2] * g2 + Rout[i - d3] * g3;

        lpL = lpf * refL + (1.0f - lpf) * lpL;
        lpR = lpf * refR + (1.0f - lpf) * lpR;

        Lout[i] += lpL;
        Rout[i] += lpR;
    }

    //7. 轻微声道交叉
    for (size_t i = 0; i < Lout.size(); ++i)
    {
        float l = Lout[i], r = Rout[i];
        Lout[i] = l * 0.85f + r * 0.15f;
        Rout[i] = r * 0.85f + l * 0.15f;
    }

    if (m_task) return false;

    // 8. 输出增益控制
    std::vector<float> output;
    size_t sz = std::min(Lout.size(), Rout.size());
    output.reserve(sz * 2);

    for (size_t i = 0; i < sz; ++i)
    {
        output.push_back(Lout[i]);
        output.push_back(Rout[i]);
    }
    if (m_task) return false;

    return writeWav(outpath.toStdString(), output, 44100, 2);
}

bool AudioSeparator::doubleEarListening(const QString& leftPath, const QString& rightPath, const QString& Lname, const QString& Rname)
{
    QString folder = QCoreApplication::applicationDirPath() + "/[double]music/" + Lname + "+" + Rname;

    WebgetCover* lweb = new WebgetCover();
    WebgetCover* rweb = new WebgetCover();

	getMusicInfo(leftPath,Lname ,lweb);
	getMusicInfo(rightPath, Rname, rweb);

    QDir().mkpath(folder);

    QString leftFile = folder + "/left.wav";

    QString rightFile = folder + "/right.wav";

    if (!QFile::exists(leftFile))
    {
        std::vector<float> stereo;

        int sampleRate = 0;

        if (!decodeAudio(
            leftPath.toUtf8().toStdString(),
            stereo,
            sampleRate))
        {
            m_lastError = "Left decode failed";
            return false;
        }

        if (stereo.empty())
        {
            m_lastError = "Left audio empty";
            return false;
        }

        std::vector<float> leftStereo;

        leftStereo.reserve(stereo.size());

        for (size_t i = 0; i < stereo.size() / 2; i++)
        {
            float left = stereo[i * 2];


            leftStereo.push_back(left);
            leftStereo.push_back(0.0f);
        }

        if (!writeWav(
            leftFile.toUtf8().toStdString(),
            leftStereo,
            sampleRate,
            2))
        {
            m_lastError = "Left write failed";
            return false;
        }
    }

    if (!QFile::exists(rightFile))
    {
        std::vector<float> stereo;

        int sampleRate = 0;

        if (!decodeAudio(rightPath.toUtf8().toStdString(), stereo, sampleRate))
        {
            m_lastError = "Right decode failed";
            return false;
        }

        if (stereo.empty())
        {
            m_lastError = "Right audio empty";
            return false;
        }

        std::vector<float> rightStereo;

        rightStereo.reserve(stereo.size());

        for (size_t i = 0; i < stereo.size() / 2; i++)
        {
            float right = stereo[i * 2 + 1];

            rightStereo.push_back(0.0f);
            rightStereo.push_back(right);
        }

        if (!writeWav(rightFile.toUtf8().toStdString(), rightStereo, sampleRate, 2))
        {
            m_lastError = "Right write failed";
            return false;
        }
    }

    return true;
}
//卷积
void AudioSeparator::Convolve(
    const std::vector<float>& input,
    const std::vector<float>& ir,
    std::vector<float>& output)
{
    output.assign(input.size() + ir.size() - 1, 0.0f);

    for (size_t n = 0; n < input.size(); ++n)
    {
        float temp = input[n];
        for (size_t k = 0; k < ir.size(); ++k)
        {
            output[n + k] += temp * ir[k];
        }
    }
}

//插值
bool AudioSeparator::getInterpolatedIR(
    float elevation,
    float azimuth,
    std::vector<float>& leftIR,
    std::vector<float>& rightIR)
{
    int eLo = 0, eHi = 0;

    //方位角插值
    HRTFData* h1Lo = nullptr, * h2Lo = nullptr;
    float aLo;
    if (!FindTwoNearestHRIR(eLo, azimuth, h1Lo, h2Lo, aLo))
        return false;
    std::vector<float> lLo(h1Lo->leftIR.size()), rLo(h1Lo->rightIR.size());
    for (size_t i = 0; i < lLo.size(); ++i)
    {
        lLo[i] = h1Lo->leftIR[i] * (1.0f - aLo) + h2Lo->leftIR[i] * aLo;
        rLo[i] = h1Lo->rightIR[i] * (1.0f - aLo) + h2Lo->rightIR[i] * aLo;
    }

    leftIR = lLo;
    rightIR = rLo;

    return true;
}

//处理声源
void AudioSeparator::ProcessVirtualSource(const std::vector<float>& inputBlock, float elevation, float azimuth, float midSideGain, size_t pos, std::vector<float>& Lout, std::vector<float>& Rout, std::vector<float>& Ldry, std::vector<float>& Rdry, int blockSize)
{
    std::vector<float> irL;
    std::vector<float> irR;

    if (!getInterpolatedIR(elevation, azimuth, irL, irR))
        return;

    std::vector<float> leftPart;
    std::vector<float> rightPart;

    leftPart.resize(blockSize + irL.size() - 1);
    rightPart.resize(blockSize + irR.size() - 1);

    // HRTF卷积
    Convolve(inputBlock, irL, leftPart);
    Convolve(inputBlock, irR, rightPart);

    // Mid-Side 展宽
    for (size_t i = 0; i < leftPart.size(); ++i)
    {
        float m = (leftPart[i] + rightPart[i]) * 0.5f;
        float s = (leftPart[i] - rightPart[i]) * midSideGain;

        leftPart[i] = m + s * 0.5f;
        rightPart[i] = m - s * 0.5f;
    }

    // ITD
    float rad = azimuth * M_PI / 180.0f;
    int itd = static_cast<int>(std::sin(rad) * 8.0f);

    // 写入干声
    for (size_t i = 0; i < leftPart.size(); ++i)
    {
        size_t idx = pos + i;

        if (idx < Ldry.size())
        {
            Ldry[idx] += leftPart[i];
            Rdry[idx] += rightPart[i];
        }
    }

    for (size_t i = 0; i < leftPart.size(); ++i)
    {
        int li = static_cast<int>(pos + i);
        int ri = static_cast<int>(pos + i);

        if (itd > 0)
            ri += itd;
        else
            li -= itd;

        if (li >= 0 && li < static_cast<int>(Lout.size()))
            Lout[li] += leftPart[i];

        if (ri >= 0 && ri < static_cast<int>(Rout.size()))
            Rout[ri] += rightPart[i];
    }
}

void AudioSeparator::getMusicInfo(const QString& path, const QString& name, WebgetCover* web)
{
    QMediaPlayer* temp = new QMediaPlayer(this);

    connect(web, &WebgetCover::coverReady, this, [name](const QString& src) {
        const QString dst = QCoreApplication::applicationDirPath() + "/cover/" + name + ".jpg";
        const QString srcCanonical = QFileInfo(src).canonicalFilePath();
        if (srcCanonical.isEmpty() ||
            srcCanonical.compare(QFileInfo(dst).canonicalFilePath(), Qt::CaseInsensitive) == 0)
            return;
        QFile::remove(dst);
        if (!QFile::rename(src, dst))
            qDebug() << "[cover rename failed]" << src << "->" << dst;
        });

    connect(web, &WebgetCover::lyricReady, this, [name](const QString& src) {
        const QString dst = QCoreApplication::applicationDirPath() + "/lrc/" + name + ".lrc";
        const QString srcCanonical = QFileInfo(src).canonicalFilePath();
        if (srcCanonical.isEmpty() ||
            srcCanonical.compare(QFileInfo(dst).canonicalFilePath(), Qt::CaseInsensitive) == 0)
            return;
        QFile::remove(dst);
        if (!QFile::rename(src, dst))
            qDebug() << "[lyric rename failed]" << src << "->" << dst;
        });

    connect(temp, &QMediaPlayer::metaDataChanged, this,
        [this, path, temp,web]()
        {
            auto meta = temp->metaData();
            QString title = meta.value(QMediaMetaData::Title).toString().trimmed();
            QString artist = meta.value(QMediaMetaData::ContributingArtist).toString().trimmed();
            if (artist.isEmpty())
                artist = meta.value(QMediaMetaData::Author).toString().trimmed();

            if (title.isEmpty() && artist.isEmpty())
            {
                temp->deleteLater();
                return;
            }
            
            float targetDuration = temp->duration() / 1000.0;
            web->searchMusicInfo(title, artist, targetDuration);

            temp->disconnect(this);
            temp->deleteLater();
        });

    temp->setSource(QUrl::fromLocalFile(path));
}

void AudioSeparator::cancelTask()
{
    m_task.store(true);
    emit sendtaskName("");
    emit separateProgress(0);
}

bool AudioSeparator::separate(const std::string& inputFile)
{
    m_task.store(false);
    emit sendtaskName(QFileInfo(QString::fromStdString(inputFile)).completeBaseName());
    qDebug() << "解码";
    //  解码
    std::vector<float> stereoPcm;
    int sampleRate = 0;
    if (!decodeAudio(inputFile, stereoPcm, sampleRate)) return false;
    if (stereoPcm.empty())
    {
        m_lastError = "No audio data";
        return false;
    }

    if (m_task) return false;
    qDebug() << "开始分离左右声道";
    //  分离左右声道
    std::vector<float> leftPcm, rightPcm;
    leftPcm.reserve(stereoPcm.size() / 2);
    rightPcm.reserve(stereoPcm.size() / 2);
    for (size_t i = 0; i < stereoPcm.size() / 2; ++i)
    {
        leftPcm.push_back(stereoPcm[2 * i]);
        rightPcm.push_back(stereoPcm[2 * i + 1]);
    }

    std::vector<float>().swap(stereoPcm);

    if (m_task) return false;
    qDebug() << "开始STFT";

    // 常量
    const int fftSize = 6144;
    const int hopSize = 1024;
    const int patchFrames = 256;
    const int patchHop = 128;
    const int targetBins = 3072;
    const int srcBins = fftSize / 2 + 1;          // 3073
    const int istftCtx = fftSize / hopSize - 1;   // 5

    // 整首歌的帧数（与旧 stft() 逐帧一致）
    int totalFrames = 0;
    if ((int)leftPcm.size() >= fftSize)
        totalFrames = (int)((leftPcm.size() - fftSize) / hopSize) + 1;
    if ((int)rightPcm.size() >= fftSize)
        totalFrames = std::min(totalFrames, (int)((rightPcm.size() - fftSize) / hopSize) + 1);

    if (totalFrames < patchFrames)
    {
        m_lastError = "Audio too short";
        return false;
    }

    const int totalOut = (totalFrames - 1) * hopSize + fftSize;

    // 块长：约 30 秒，按 patchHop 对齐
    int chunkFrames = (int)((double)(30 * sampleRate) / hopSize);
    chunkFrames = (chunkFrames / patchHop) * patchHop;
    if (chunkFrames < patchFrames) chunkFrames = patchFrames;

    const int jMax = (totalFrames - patchFrames) / patchHop;

    // 输出与临时文件
    QString humanVoiceFile = QCoreApplication::applicationDirPath() + "/pureHumanVoice/"
        + QFileInfo(QString::fromStdString(inputFile)).fileName();
    QString accompFile = QCoreApplication::applicationDirPath() + "/pureAccompaniment/"
        + QFileInfo(QString::fromStdString(inputFile)).fileName();
    QDir().mkdir(QCoreApplication::applicationDirPath() + "/pureHumanVoice");
    QDir().mkdir(QCoreApplication::applicationDirPath() + "/pureAccompaniment");

    // 人声谱落临时文件，第二遍读回（保证与整首一次算完逐字节一致）
    QString tmpDir = QCoreApplication::applicationDirPath() + "/tmp";
    QDir().mkdir(tmpDir);
    QString tmpVoiceSpec = tmpDir + "/"
        + QFileInfo(QString::fromStdString(inputFile)).completeBaseName() + ".voicespec";

    QFile specFile(tmpVoiceSpec);
    if (!specFile.open(QIODevice::ReadWrite | QIODevice::Truncate))
    {
        m_lastError = "无法创建临时谱文件";
        return false;
    }

    Ort::Session* session = static_cast<Ort::Session*>(m_session);
    Ort::MemoryInfo* memInfo = static_cast<Ort::MemoryInfo*>(m_memInfo);

    int lastProgress = -1;
    auto report = [&](int v)
    {
        if (v != lastProgress)
        {
            lastProgress = v;
            emit separateProgress(v);
        }
    };

    WavStreamWriter accompWriter;
    if (!accompWriter.open(accompFile, sampleRate, 2))
    {
        specFile.close();
        QFile::remove(tmpVoiceSpec);
        m_lastError = "无法写入伴奏文件";
        return false;
    }

    const int patchSize = targetBins * patchFrames;
    std::vector<float> inputTensor;

    double magSum = 0.0;
    const int noiseFrames = std::min(5, totalFrames / 2);
    std::vector<float> noiseMagL, noiseMagR;
    bool failed = false;

    // 第一遍：分块 STFT + ONNX + 平均 → 伴奏落盘 / 人声谱落盘 / 全局统计
    for (int C0 = 0; C0 < totalFrames && !failed; C0 += chunkFrames)
    {
        if (m_task) { failed = true; break; }

        const int C1 = std::min(C0 + chunkFrames, totalFrames);
        const int U0 = std::max(0, C0 - istftCtx);   // 需要完整覆盖的帧区间（含 ISTFT 左上下文）
        const int U1 = C1;
        const int Uc = U1 - U0;

        const int jLo = std::max(0, (U0 / patchHop) - 1);
        const int jHi = std::min((U1 - 1) / patchHop, jMax);

        const int Fs = std::min(U0, jLo * patchHop);
        int Fe = std::max(U1, jHi * patchHop + patchFrames);
        if (Fe > totalFrames) Fe = totalFrames;

        auto chunkL = stftRange(leftPcm, Fs, Fe, fftSize, hopSize);
        auto chunkR = stftRange(rightPcm, Fs, Fe, fftSize, hopSize);
        if ((int)chunkL.size() != Fe - Fs || (int)chunkR.size() != Fe - Fs)
        {
            m_lastError = "STFT failed";
            failed = true;
            break;
        }

        std::vector<std::complex<float>> accL((size_t)srcBins * Uc, 0.0f);
        std::vector<std::complex<float>> accR((size_t)srcBins * Uc, 0.0f);
        std::vector<uint8_t> cnt((size_t)srcBins * Uc, 0);

        for (int j = jLo; j <= jHi; ++j)
        {
            if (m_task) { failed = true; break; }

            const int start = j * patchHop;
            buildInputTensor(chunkL, chunkR, start - Fs, inputTensor);

            std::vector<int64_t> inputShape = { 1, 4, targetBins, patchFrames };
            Ort::Value inputValue = Ort::Value::CreateTensor<float>(
                *memInfo, inputTensor.data(), inputTensor.size(),
                inputShape.data(), inputShape.size());
            const char* inputNames[] = { m_inputName.c_str() };
            const char* outputNames[] = { m_outputName.c_str() };
            auto outputs = session->Run(Ort::RunOptions(), inputNames, &inputValue, 1, outputNames, 1);

            float* ptr = outputs[0].GetTensorMutableData<float>();
            size_t count = outputs[0].GetTensorTypeAndShapeInfo().GetElementCount();
            std::vector<float> outputSpectrum(ptr, ptr + count);

            for (int t = 0; t < patchFrames; ++t)
            {
                const int gT = start + t;
                if (gT < U0 || gT >= U1) continue;
                const int li = gT - U0;
                for (int f = 0; f < targetBins; ++f)
                {
                    const int pi = f * patchFrames + t;
                    const int idx = li * srcBins + f;
                    accL[idx] += std::complex<float>(outputSpectrum[0 * patchSize + pi],
                        outputSpectrum[1 * patchSize + pi]);
                    accR[idx] += std::complex<float>(outputSpectrum[2 * patchSize + pi],
                        outputSpectrum[3 * patchSize + pi]);
                    cnt[idx]++;
                }
                // 第 3073 个 bin（Nyquist）保持 0，计数与原实现一致
                cnt[li * srcBins + targetBins]++;
            }
        }
        if (failed) break;

        // 平均 → 本块人声谱
        std::vector<STFTFrame> outL(Uc), outR(Uc);
        for (int t = 0; t < Uc; ++t)
        {
            outL[t].spectrum.resize(srcBins);
            outR[t].spectrum.resize(srcBins);
            for (int f = 0; f < srcBins; ++f)
            {
                const int idx = t * srcBins + f;
                if (cnt[idx] > 0)
                {
                    outL[t].spectrum[f] = accL[idx] / (float)cnt[idx];
                    outR[t].spectrum[f] = accR[idx] / (float)cnt[idx];
                }
                else
                {
                    outL[t].spectrum[f] = 0.0f;
                    outR[t].spectrum[f] = 0.0f;
                }
            }
        }
        std::vector<std::complex<float>>().swap(accL);
        std::vector<std::complex<float>>().swap(accR);
        std::vector<uint8_t>().swap(cnt);

        // 伴奏 = 原谱 − 人声谱（就地写回 chunkL/chunkR，省一份容器）
        const int off = U0 - Fs;
        for (int t = 0; t < Uc; ++t)
        {
            auto& dL = chunkL[off + t].spectrum;
            auto& dR = chunkR[off + t].spectrum;
            const auto& vL = outL[t].spectrum;
            const auto& vR = outR[t].spectrum;
            for (int f = 0; f < srcBins; ++f)
            {
                dL[f] -= vL[f];
                dR[f] -= vR[f];
            }
        }

        // 伴奏 ISTFT → 追加落盘（伴奏不参与归一化，此刻即可定稿）
        const int oS = C0 * hopSize;
        const int oE = (C1 == totalFrames) ? totalOut : (C1 * hopSize);
        auto accompPcmL = istftRange(chunkL, Fs, oS, oE, fftSize, hopSize);
        auto accompPcmR = istftRange(chunkR, Fs, oS, oE, fftSize, hopSize);
        std::vector<float> interleaved((size_t)(oE - oS) * 2);
        for (size_t i = 0; i < accompPcmL.size(); ++i)
        {
            interleaved[2 * i] = accompPcmL[i];
            interleaved[2 * i + 1] = accompPcmR[i];
        }
        if (!accompWriter.append(interleaved))
        {
            m_lastError = "伴奏写入失败";
            failed = true;
            break;
        }
        std::vector<float>().swap(accompPcmL);
        std::vector<float>().swap(accompPcmR);
        std::vector<float>().swap(interleaved);
        std::vector<AudioSeparator::STFTFrame>().swap(chunkL);
        std::vector<AudioSeparator::STFTFrame>().swap(chunkR);

        // 累加全局平均幅度（只统计 [C0,C1)，保证每帧只计一次，求和次序与原实现一致）
        for (int t = C0; t < C1; ++t)
        {
            const auto& sL = outL[t - U0].spectrum;
            const auto& sR = outR[t - U0].spectrum;
            for (int f = 0; f < srcBins; ++f)
            {
                magSum += std::abs(sL[f]);
                magSum += std::abs(sR[f]);
            }
        }

        // 第 1 块顺手取噪声谱（原 :1224-1237）
        if (C0 == 0)
        {
            noiseMagL.assign(srcBins, 0.0f);
            noiseMagR.assign(srcBins, 0.0f);
            for (int f = 0; f < srcBins; ++f)
            {
                float sumL = 0.0f, sumR = 0.0f;
                for (int t = 0; t < noiseFrames; ++t)
                {
                    sumL += std::abs(outL[t].spectrum[f]);
                    sumR += std::abs(outR[t].spectrum[f]);
                }
                noiseMagL[f] = sumL / noiseFrames;
                noiseMagR[f] = sumR / noiseFrames;
            }
        }

        // 人声谱顺序落盘（每帧先左后右）
        {
            std::vector<std::complex<float>> buf;
            buf.resize((size_t)(C1 - C0) * 2 * srcBins);
            for (int t = C0; t < C1; ++t)
            {
                const int li = t - U0;
                std::copy(outL[li].spectrum.begin(), outL[li].spectrum.end(),
                    buf.begin() + (size_t)(t - C0) * 2 * srcBins);
                std::copy(outR[li].spectrum.begin(), outR[li].spectrum.end(),
                    buf.begin() + ((size_t)(t - C0) * 2 + 1) * srcBins);
            }
            if (specFile.write(reinterpret_cast<const char*>(buf.data()),
                    (qint64)buf.size() * (qint64)sizeof(std::complex<float>)) < 0)
            {
                m_lastError = "临时谱写入失败";
                failed = true;
                break;
            }
        }

        report((int)(60.0 * C1 / totalFrames));
    }

    if (!failed && m_task) failed = true;

    if (failed)
    {
        accompWriter.close();
        specFile.close();
        QFile::remove(tmpVoiceSpec);
        return false;
    }

    accompWriter.close();
    specFile.flush();

    const float globalAvgMag = (float)(magSum / (2.0 * (double)srcBins * (double)totalFrames));

    // 源 PCM 在第一遍后不再需要
    std::vector<float>().swap(leftPcm);
    std::vector<float>().swap(rightPcm);

    // 第二遍：读回人声谱 → 谱减/软阈值/高通 → ISTFT → 拼整首
    std::vector<float> stereoOut((size_t)totalOut * 2, 0.0f);

    const float specAlpha = 1.9f;      // 谱减法过减因子
    const float specBeta = 0.005f;     // 谱底噪声保留系数
    const float thresholdRatio = 0.1f; // 软阈值系数
    const float attenuation = 0.2f;    // 软阈值衰减系数
    const float cutoffFreq = 250.0f;   // 高通截止频率
    const qint64 frameBytes = (qint64)2 * srcBins * (qint64)sizeof(std::complex<float>);

    for (int C0 = 0; C0 < totalFrames && !failed; C0 += chunkFrames)
    {
        if (m_task) { failed = true; break; }

        const int C1 = std::min(C0 + chunkFrames, totalFrames);
        const int U0 = std::max(0, C0 - istftCtx);
        const int U1 = C1;
        const int Uc = U1 - U0;

        specFile.seek((qint64)U0 * frameBytes);
        std::vector<std::complex<float>> buf((size_t)Uc * 2 * srcBins);
        const qint64 want = (qint64)buf.size() * (qint64)sizeof(std::complex<float>);
        if (specFile.read(reinterpret_cast<char*>(buf.data()), want) != want)
        {
            m_lastError = "临时谱读取失败";
            failed = true;
            break;
        }

        std::vector<STFTFrame> outL(Uc), outR(Uc);
        for (int t = 0; t < Uc; ++t)
        {
            outL[t].spectrum.resize(srcBins);
            outR[t].spectrum.resize(srcBins);
            std::copy(buf.begin() + (size_t)t * 2 * srcBins,
                buf.begin() + (size_t)t * 2 * srcBins + srcBins,
                outL[t].spectrum.begin());
            std::copy(buf.begin() + ((size_t)t * 2 + 1) * srcBins,
                buf.begin() + ((size_t)t * 2 + 2) * srcBins,
                outR[t].spectrum.begin());
        }
        std::vector<std::complex<float>>().swap(buf);

        // 谱减 + 软阈值 + 高通（逐格独立，合并为一趟，与原分两趟等价）
        for (int t = 0; t < Uc; ++t)
        {
            auto& sL = outL[t].spectrum;
            auto& sR = outR[t].spectrum;
            for (int f = 0; f < srcBins; ++f)
            {
                const float magL = std::abs(sL[f]);
                const float magR = std::abs(sR[f]);
                const float newMagL = std::max(magL - specAlpha * noiseMagL[f], specBeta * noiseMagL[f]);
                const float newMagR = std::max(magR - specAlpha * noiseMagR[f], specBeta * noiseMagR[f]);
                if (magL > 1e-6f) sL[f] *= (newMagL / magL);
                if (magR > 1e-6f) sR[f] *= (newMagR / magR);

                const float fMagL = std::abs(sL[f]);
                const float fMagR = std::abs(sR[f]);
                if (fMagL < globalAvgMag * thresholdRatio) sL[f] *= attenuation;
                if (fMagR < globalAvgMag * thresholdRatio) sR[f] *= attenuation;

                const float freq = (float)f * 44100.0f / fftSize;
                if (freq < cutoffFreq)
                {
                    const float gain = std::pow(freq / cutoffFreq, 2.0f);
                    sL[f] *= gain;
                    sR[f] *= gain;
                }
            }
        }

        const int oS = C0 * hopSize;
        const int oE = (C1 == totalFrames) ? totalOut : (C1 * hopSize);
        auto pcmL = istftRange(outL, U0, oS, oE, fftSize, hopSize);
        auto pcmR = istftRange(outR, U0, oS, oE, fftSize, hopSize);
        for (int i = 0; i < oE - oS; ++i)
        {
            stereoOut[(size_t)2 * (oS + i)] = pcmL[i];
            stereoOut[(size_t)2 * (oS + i) + 1] = pcmR[i];
        }

        report(60 + (int)(35.0 * C1 / totalFrames));
    }

    specFile.close();
    QFile::remove(tmpVoiceSpec);

    if (failed || m_task) return false;

    // 归一化
    float maxAbs = 0.0f;
    for (float s : stereoOut) maxAbs = std::max(maxAbs, std::fabs(s));
    if (maxAbs > 0.0f)
    {
        const float targetPeak = 0.98f;
        const float gain = targetPeak / maxAbs;
        for (float& s : stereoOut) s *= gain;
    }
    for (float& s : stereoOut)
    {
        if (s > 1.0f) s = 1.0f;
        if (s < -1.0f) s = -1.0f;
    }

    bool f = writeWav(humanVoiceFile.toUtf8().toStdString(), stereoOut, sampleRate, 2);
    emit separateProgress(100);

    QTimer::singleShot(1500, this, [this]()
    {
        emit sendtaskName("");
        emit separateProgress(0);
    });

    return f;
}

bool AudioSeparator::FindTwoNearestHRIR(int elevation, float azimuth, HRTFData*& h1, HRTFData*& h2, float& alpha)
{
    h1 = nullptr;
    h2 = nullptr;

    // 只寻找同一俯仰角

    std::vector<HRTFData*> list;

    for (auto& hrir : wavFiles)
    {
        if (hrir.elevation == elevation)
        {
            list.push_back(&hrir);
        }
    }

    if (list.size() < 2)
    {
        return false;
    }

    // 按方位角排序
    std::sort(
        list.begin(),
        list.end(),
        [](HRTFData* a, HRTFData* b)
        {
            return a->azimuth < b->azimuth;
        });

    // 找到包围当前角度的两个HRIR

    for (size_t i = 0; i < list.size() - 1; i++)
    {
        if (azimuth >= list[i]->azimuth &&
            azimuth <= list[i + 1]->azimuth)
        {
            h1 = list[i];
            h2 = list[i + 1];
            break;
        }
    }

    // 首尾连接（360°）
    if (h1 == nullptr)
    {
        h1 = list.back();
        h2 = list.front();
    }

    // 计算插值系数
    float a1 = static_cast<float>(h1->azimuth);
    float a2 = static_cast<float>(h2->azimuth);

    // 处理360°跨越
    if (a2 < a1)
    {
        a2 += 360.0f;

        if (azimuth < a1)
        {
            azimuth += 360.0f;
        }
    }

    alpha =
        (azimuth - a1) /
        (a2 - a1);

    alpha = std::clamp(alpha, 0.0f, 1.0f);

    return true;
}

