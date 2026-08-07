#include "HeaderFile/AudioSeparator.h"
#include "HeaderFile/LoadlocalMusic.h"

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

std::vector<AudioSeparator::STFTFrame> AudioSeparator::stft(const std::vector<float>& pcm, int fftSize, int hopSize) 
{
    std::vector<STFTFrame> frames;
    kiss_fft_cfg cfg = kiss_fft_alloc(fftSize, 0, nullptr, nullptr);
    if (!cfg) return frames;

    std::vector<float> window(fftSize);
    for (int i = 0; i < fftSize; ++i)
        window[i] = 0.54f - 0.46f * cos(2.0 * M_PI * i / (fftSize - 1));

    int specSize = fftSize / 2 + 1;
    std::vector<kiss_fft_cpx> in(fftSize);
    std::vector<kiss_fft_cpx> out(fftSize);

    for (size_t start = 0; start + fftSize <= pcm.size(); start += hopSize) 
    {
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

std::vector<float> AudioSeparator::istft(const std::vector<STFTFrame>& frames, int fftSize, int hopSize) 
{
    if (frames.empty()) return {};
    kiss_fft_cfg cfg = kiss_fft_alloc(fftSize, 1, nullptr, nullptr);
    if (!cfg) return {};

    int outputSize = (frames.size() - 1) * hopSize + fftSize;
    std::vector<float> output(outputSize, 0.0f);
    std::vector<float> weight(outputSize, 0.0f);

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
        int start = fi * hopSize;
        for (int n = 0; n < fftSize; ++n) 
        {
            float s = out[n].r / fftSize;
            s *= window[n];
            output[start + n] += s;
            weight[start + n] += window[n] * window[n];
        }
    }
    for (size_t i = 0; i < output.size(); ++i) 
    {
        if (weight[i] > 1e-6f)
            output[i] /= weight[i];
    }
    free(cfg);
    return output;
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

    emit surroundfished();

    return writeWav(outpath.toStdString(), output, 44100, 2);
}

bool AudioSeparator::doubleEarListening(const QString& leftPath, const QString& rightPath, const QString& Lname, const QString& Rname)
{
    loadlocalMusic loadMusic;

    loadMusic.addMusic(rightPath);
    loadMusic.addMusic(leftPath);

    QString folder = QCoreApplication::applicationDirPath() + "/[double]music/" + Lname + "+" + Rname;

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

void AudioSeparator::cancelTask()
{
    m_task.store(true);
    emit sendtaskName("");
    emit separateProgress(0);
}

bool AudioSeparator::LoadIR(int elevation, float azimuth, std::vector<float>& leftIR, std::vector<float>& rightIR)
{
    HRTFData* h1;
    HRTFData* h2;

    float alpha;

    if (!FindTwoNearestHRIR(
        elevation,
        azimuth,
        h1,
        h2,
        alpha))
    {
        return false;
    }

    leftIR.resize(h1->leftIR.size());
    rightIR.resize(h1->rightIR.size());

    for (size_t i = 0; i < leftIR.size(); i++)
    {
        leftIR[i] =
            h1->leftIR[i] * (1.0f - alpha) +
            h2->leftIR[i] * alpha;

        rightIR[i] =
            h1->rightIR[i] * (1.0f - alpha) +
            h2->rightIR[i] * alpha;
    }

    return true;
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

    if (m_task) return false;
    qDebug() << "开始STFT";
    // STFT
    int fftSize = 6144;
    int hopSize = 1024;
    auto leftFrames = stft(leftPcm, fftSize, hopSize);
    auto rightFrames = stft(rightPcm, fftSize, hopSize);
    if (leftFrames.empty() || rightFrames.empty()) 
    {
        m_lastError = "STFT failed";
        return false;
    }

    int totalFrames = (int)leftFrames.size();
    int srcBins = (int)leftFrames[0].spectrum.size(); // 3073
    const int patchFrames = 256;
    const int patchHop = 128;
    const int targetBins = 3072;

    if (totalFrames < patchFrames)
    {
        m_lastError = "Audio too short";
        return false;
    }

    if (m_task) return false;

    // 累加器
    std::vector<std::complex<float>> leftSpecAcc(srcBins * totalFrames, 0.0f);
    std::vector<std::complex<float>> rightSpecAcc(srcBins * totalFrames, 0.0f);
    std::vector<int> specCount(srcBins * totalFrames, 0);

    Ort::Session* session = static_cast<Ort::Session*>(m_session);
    Ort::MemoryInfo* memInfo = static_cast<Ort::MemoryInfo*>(m_memInfo);

    static int lastProgress = -1;

    std::vector<float> inputTensor;
    const int patchSize = targetBins * patchFrames;

    std::vector<std::complex<float>> leftPatch(patchSize);
    std::vector<std::complex<float>> rightPatch(patchSize);

    if (m_task) return false;

    for (int start = 0; start + patchFrames <= totalFrames; start += patchHop)
    {
        if (m_task) return false;
        buildInputTensor(leftFrames, rightFrames, start, inputTensor);

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

        for (int i = 0; i < patchSize; ++i) 
        {
            leftPatch[i] = std::complex<float>(outputSpectrum[0 * patchSize + i],
                outputSpectrum[1 * patchSize + i]);
            rightPatch[i] = std::complex<float>(outputSpectrum[2 * patchSize + i],
                outputSpectrum[3 * patchSize + i]);
        }

        // 官方补零扩展
        std::vector<std::complex<float>> leftFrame(targetBins);
        std::vector<std::complex<float>> rightFrame(targetBins);    
        std::vector<std::complex<float>> leftExtended(srcBins, 0);
        std::vector<std::complex<float>> rightExtended(srcBins, 0); 
        if (m_task) return false;
        for (int t = 0; t < patchFrames; ++t) 
        {
            for (int f = 0; f < targetBins; ++f) 
            {
                leftFrame[f] = leftPatch[f * patchFrames + t];
                rightFrame[f] = rightPatch[f * patchFrames + t];
            }
            if (m_task) return false;
            std::copy(leftFrame.begin(), leftFrame.end(), leftExtended.begin());
            std::copy(rightFrame.begin(), rightFrame.end(), rightExtended.begin());

            std::fill(leftExtended.begin() + targetBins, leftExtended.end(), std::complex<float>(0.0f, 0.0f));
            std::fill(rightExtended.begin() + targetBins, rightExtended.end(), std::complex<float>(0.0f, 0.0f));

            int globalT = start + t;
            if (globalT >= totalFrames) break;
            
            if (m_task) return false;
            
            for (int f = 0; f < srcBins; ++f) 
            {
                int idx = globalT * srcBins + f;

                leftSpecAcc[idx] += leftExtended[f];
                rightSpecAcc[idx] += rightExtended[f];
                specCount[idx]++;
            }
        }
        //进度条
        int progress = static_cast<int>(std::min(100.0, 100.0 * (start + patchFrames) / totalFrames));

        if (progress != lastProgress)
        {
            lastProgress = progress;
            emit separateProgress(progress);
        }
        if (m_task) return false;
    }
    
    if (m_task) return false;
    
    // 平均得到频谱
    std::vector<STFTFrame> leftOutFrames(totalFrames);
    std::vector<STFTFrame> rightOutFrames(totalFrames);        
    for (int t = 0; t < totalFrames; ++t)
    {
        if (m_task) return false;
        leftOutFrames[t].spectrum.resize(srcBins);
        rightOutFrames[t].spectrum.resize(srcBins);
    }
    for (int t = 0; t < totalFrames; ++t) 
    {
        if (m_task) return false;
        for (int f = 0; f < srcBins; ++f) 
        {
            if (m_task) return false;
            int idx = t * srcBins + f;
            if (specCount[idx] > 0) 
            {
                leftOutFrames[t].spectrum[f] = leftSpecAcc[idx] / (float)specCount[idx];
                rightOutFrames[t].spectrum[f] = rightSpecAcc[idx] / (float)specCount[idx];
            }
            else 
            {
                leftOutFrames[t].spectrum[f] = 0.0f;
                rightOutFrames[t].spectrum[f] = 0.0f;
            }
            if (m_task) return false;
        }
        if (m_task) return false;
    }
    std::vector<STFTFrame> leftCopy = leftOutFrames;
    std::vector<STFTFrame> rightCopy = rightOutFrames;

    //伴奏容器
    std::vector<STFTFrame> leftAccompFrames(totalFrames);
    std::vector<STFTFrame> rightAccompFrames(totalFrames);
    if (m_task) return false;
    for (int i = 0; i < totalFrames; i++)
    {
        leftAccompFrames[i].spectrum.resize(srcBins);
        rightAccompFrames[i].spectrum.resize(srcBins);
        for (int j = 0; j < srcBins; j++)
        {
            std::complex<float> leftVal = leftFrames[i].spectrum[j];
            std::complex<float> rightVal = rightFrames[i].spectrum[j];

            leftAccompFrames[i].spectrum[j] = leftVal - leftCopy[i].spectrum[j];
            rightAccompFrames[i].spectrum[j] = rightVal - rightCopy[i].spectrum[j];
        }
    }
    if (m_task) return false;
    // 增强降噪处理 
    // 估算噪声谱：取前 5 帧的平均幅度
    const int noiseFrames = std::min(5, totalFrames / 2);
    std::vector<float> noiseMagL(srcBins, 0.0f);
    std::vector<float> noiseMagR(srcBins, 0.0f);
    for (int f = 0; f < srcBins; ++f) 
    {
        float sumL = 0.0f, sumR = 0.0f;
        for (int t = 0; t < noiseFrames; ++t) 
        {
            sumL += std::abs(leftOutFrames[t].spectrum[f]);
            sumR += std::abs(rightOutFrames[t].spectrum[f]);
        }
        noiseMagL[f] = sumL / noiseFrames;
        noiseMagR[f] = sumR / noiseFrames;
    }
    if (m_task) return false;
    // 谱减法 + 软阈值
    float alpha = 1.9f;      // 谱减法过减因子
    float beta = 0.005f;      // 谱底噪声保留系数
    float thresholdRatio = 0.1f;   // 软阈值系数
    float attenuation = 0.2f;      // 软阈值衰减系数

    // 先计算全局平均幅度（用于软阈值）
    double totalMag = 0.0;
    int totalCells = srcBins * totalFrames;
    for (int t = 0; t < totalFrames; ++t) 
    {
        for (int f = 0; f < srcBins; ++f) 
        {
            totalMag += std::abs(leftOutFrames[t].spectrum[f]);
            totalMag += std::abs(rightOutFrames[t].spectrum[f]);
        }
    }
    float globalAvgMag = (float)(totalMag / (2.0 * totalCells));

    for (int t = 0; t < totalFrames; ++t) 
    {
        if (m_task) return false;
        for (int f = 0; f < srcBins; ++f) 
        {
            if (m_task) return false;
            // 谱减法：估计信号幅度 = max(当前幅度 - alpha * 噪声幅度, beta * 噪声幅度)
            float magL = std::abs(leftOutFrames[t].spectrum[f]);
            float magR = std::abs(rightOutFrames[t].spectrum[f]);
            float newMagL = std::max(magL - alpha * noiseMagL[f], beta * noiseMagL[f]);
            float newMagR = std::max(magR - alpha * noiseMagR[f], beta * noiseMagR[f]);
            // 保持相位不变，缩放复数
            if (magL > 1e-6f) 
            {
                leftOutFrames[t].spectrum[f] *= (newMagL / magL);
            }
            if (magR > 1e-6f) 
            {
                rightOutFrames[t].spectrum[f] *= (newMagR / magR);
            }

            // 软阈值：进一步抑制极低幅度的残留噪声
            float finalMagL = std::abs(leftOutFrames[t].spectrum[f]);
            float finalMagR = std::abs(rightOutFrames[t].spectrum[f]);
            if (finalMagL < globalAvgMag * thresholdRatio) 
            {
                leftOutFrames[t].spectrum[f] *= attenuation;
            }
            if (finalMagR < globalAvgMag * thresholdRatio) 
            {
                rightOutFrames[t].spectrum[f] *= attenuation;
            }
        }
    }
    if (m_task) return false;
    // 高通滤波减少低频嗡声
    float cutoffFreq = 250.0f;   // Hz，可调整
    for (int t = 0; t < totalFrames; ++t) 
    {
        for (int f = 0; f < srcBins; ++f) 
        {
            float freq = (float)f * 44100.0f / fftSize;
            if (freq < cutoffFreq) 
            {
                // 平滑衰减，避免突变
                float gain = std::pow(freq / cutoffFreq, 2.0f);
                leftOutFrames[t].spectrum[f] *= gain;
                rightOutFrames[t].spectrum[f] *= gain;
            }
        }
    }
    if (m_task) return false;
    // ISTFT
    auto leftPcmOut = istft(leftOutFrames, fftSize, hopSize);
    auto rightPcmOut = istft(rightOutFrames, fftSize, hopSize);

    auto leftAccompPcmOut = istft(leftAccompFrames, fftSize, hopSize);
    auto rightAccompPcmOut = istft(rightAccompFrames, fftSize, hopSize);

    if (leftPcmOut.empty() || rightPcmOut.empty()) 
    {
        m_lastError = "ISTFT failed";
        return false;
    }
    if (m_task) return false;
    // 合并立体声
    std::vector<float> stereoOut;
    std::vector<float> stereoAccompOut;
    stereoOut.reserve(leftPcmOut.size() * 2);
    stereoAccompOut.reserve(leftPcmOut.size() * 2);
    for (size_t i = 0; i < leftPcmOut.size(); ++i) 
    {
        stereoOut.push_back(leftPcmOut[i]);
        stereoOut.push_back(rightPcmOut[i]);
        stereoAccompOut.push_back(leftAccompPcmOut[i]);
        stereoAccompOut.push_back(rightAccompPcmOut[i]);
    }
    if (m_task) return false;
    // 音量归一化：提升到目标峰值并限幅
    float maxAbs = 0.0f;
    for (float s : stereoOut) maxAbs = std::max(maxAbs, std::fabs(s));
    if (maxAbs > 0.0f) 
    {
        float targetPeak = 0.98f;   // 提高音量
        float gain = targetPeak / maxAbs;
        for (float& s : stereoOut) s *= gain;
    }
    // 限幅防止削波
    for (float& s : stereoOut) 
    {
        if (s > 1.0f) s = 1.0f;
        if (s < -1.0f) s = -1.0f;
    }

    QString humanVoiceFile = QCoreApplication::applicationDirPath() + "/pureHumanVoice/" 
        + QFileInfo(QString::fromStdString(inputFile)).fileName();
    QString accompFile = QCoreApplication::applicationDirPath() + "/pureAccompaniment/"
        + QFileInfo(QString::fromStdString(inputFile)).fileName();
    QDir().mkdir(QCoreApplication::applicationDirPath() + "/pureHumanVoice");
    QDir().mkdir(QCoreApplication::applicationDirPath() + "/pureAccompaniment");
    if (m_task) return false;

    emit separatefished();

    bool f = writeWav(humanVoiceFile.toUtf8().toStdString(), stereoOut, sampleRate, 2)
        && writeWav(accompFile.toUtf8().toStdString(), stereoAccompOut, sampleRate, 2);
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
