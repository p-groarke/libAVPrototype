/*
 *
 * audioplayer.h
 *
 *     Written by Philippe Groarke - February 2015
 *
 * Legal Terms:
 *
 *     This source file is released into the public domain.  It is
 *     distributed without any warranty; without even the implied
 *     warranty * of merchantability or fitness for a particular
 *     purpose.
 *
 * Function:
 *
 *
 */
#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <QObject>
#include <QDebug>

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

#include <ao/ao.h>

extern "C" {
#include <libavresample/avresample.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
}

class AudioPlayer : public QObject
{
    Q_OBJECT
public:
    AudioPlayer();
    ~AudioPlayer();

    void setFile(QString& s);
    void start();
    void stop();

private:
    void setupResampler();
    void readAudioFile();
    int init_converted_samples(uint8_t ***converted_input_samples,
            int frame_size);
    int convert_samples(uint8_t **input_data,
            uint8_t **converted_data, const int inframe_size,
            const int outframe_size);

    AVFormatContext* audio_context = NULL;
    AVCodecContext* codec_context = NULL;
    AVCodec* codec = NULL;
    AVAudioResampleContext* resample_context = NULL;
    ao_device* outputDevice = NULL;
    ao_sample_format outputFormat;

    std::atomic<bool> readingFile;
    std::thread fileRead;
};

#endif // AUDIOPLAYER_H
