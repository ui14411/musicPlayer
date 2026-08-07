#include "HeaderFile/AudioAnalyzer.h"

extern "C"
{
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libswresample/swresample.h>
    #include "kiss_fft.h"
}

#include <QVariantList>
#include <QDebug>
#include <QFile>
#include <cmath>
#include <QThread>

AudioAnalyzer::AudioAnalyzer(QObject* parent)
	:QObject(parent)
{

}

bool AudioAnalyzer::loadMusic(QString path)
{
    qDebug()
        << "AudioAnalyzer thread:"
        << QThread::currentThread();
    if (!QFile(path).exists())
    {
        qDebug() << "music not exists";
        return false;
    }

    m_musicPath = path;

    std::vector<float> pcm;

    if (!decodeAudio(path.toStdString(), pcm, m_sampleRate))
    {
        qDebug() << "decode failed";
        return false;
    }

    m_pcm = std::move(pcm);

    return true;
}

bool AudioAnalyzer::decodeAudio(const std::string& filePath,std::vector<float>& stereoPcm,int& sampleRate)
{
    stereoPcm.clear();
    AVFormatContext* fmtCtx = nullptr;
    if (avformat_open_input(&fmtCtx, filePath.c_str(), nullptr, nullptr) < 0)
    {
        m_lastError = "Failed to open file";
        return false;
    }
    if (avformat_find_stream_info(fmtCtx, nullptr) < 0)
    {
        avformat_close_input(&fmtCtx);
        m_lastError = "Failed to find stream info";
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
        return false;
    }

    AVCodecParameters* codecPar = fmtCtx->streams[audioIdx]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecPar->codec_id);
    if (!codec)
    {
        avformat_close_input(&fmtCtx);
        m_lastError = "Decoder not found";
        return false;
    }

    AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx, codecPar);
    if (avcodec_open2(codecCtx, codec, nullptr) < 0)
    {
        avcodec_free_context(&codecCtx);
        avformat_close_input(&fmtCtx);
        m_lastError = "Failed to open codec";
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

void AudioAnalyzer::analyzePCM(qint64 position)
{
    if (m_pcm.empty())
        return;

    int fftSize = 1024;

    int center = position * m_sampleRate / 1000;

    int start = center - fftSize / 2;

    if (start < 0)
        start = 0;

    std::vector<float> window(fftSize);

    for (int i = 0; i < fftSize; i++)
    {
        window[i] = 0.5f - 0.5f * cos(2 * M_PI * i / (fftSize - 1));
    }

    kiss_fft_cfg cfg = kiss_fft_alloc(fftSize, 0, nullptr, nullptr);

    std::vector<kiss_fft_cpx> in(fftSize);

    std::vector<kiss_fft_cpx> out(fftSize);

    for (int i = 0; i < fftSize; i++)
    {
        float sample = 0;

        int index = start + i * 2;

        if (index < m_pcm.size())
        {
            sample = m_pcm[index];
        }

        in[i].r = sample * window[i];

        in[i].i = 0;
    }

    kiss_fft(cfg, in.data(), out.data());

    QVariantList spectrum;

    for (int i = 0; i < fftSize / 8; i++)
    {
        float mag = sqrt(out[i].r * out[i].r + out[i].i * out[i].i);

        mag = log10(1 + mag);

        spectrum.append(mag);
    }

    free(cfg);

    emit spectrumChanged(spectrum);
}
