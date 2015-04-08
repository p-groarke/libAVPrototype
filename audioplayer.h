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
 */
#pragma once

#include <QObject>
#include <QDebug>

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

#include <ao/ao.h>

#include "globals.h"

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

    AVFormatContext* audio_context_ = NULL;
    AVCodecContext* codec_context_ = NULL;
    AVCodec* codec_ = NULL;
    AVAudioResampleContext* resample_context_ = NULL;
    ao_device* outputDevice_ = NULL;
    ao_sample_format outputFormat_;

    std::atomic<bool> readingFile_;
    std::thread fileRead_;
};
