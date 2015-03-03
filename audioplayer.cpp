/*
 *
 * audioplayer.cpp
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
#include "audioplayer.h"

AudioPlayer::AudioPlayer() :
    readingFile_(false)
{
    // libAV
    av_register_all();

    // libao http://xiph.org/ao/doc/ao_example.c
    ao_initialize();
    int default_driver = ao_default_driver_id();

    ao_info* info = ao_driver_info(ao_default_driver_id());


    memset(&outputFormat_, 0, sizeof(outputFormat_));
    outputFormat_.bits = 16;
    outputFormat_.channels = 2;
    outputFormat_.rate = sample_rate;
    outputFormat_.byte_format = info->preferred_byte_format;
    outputFormat_.matrix = 0;

    outputDevice_ = ao_open_live(default_driver, &outputFormat_,
            NULL /* no options */);
    if (outputDevice_ == NULL) {
        fprintf(stderr, "Error opening device.\n");
        return;
    }
}

AudioPlayer::~AudioPlayer()
{
    readingFile_ = false;
    if (fileRead_.joinable()) {
        fileRead_.join();
        qDebug() << "Joined";
    }

    if (audio_context_ != NULL)
        avformat_close_input(&audio_context_);

    if (resample_context_ != NULL)
        avresample_close(resample_context_);

    ao_close(outputDevice_);
    ao_shutdown();
}

// Example https://libav.org/doxygen/master/transcode_aac_8c-example.html
void AudioPlayer::setFile(QString &s)
{
    if (audio_context_ != NULL)
        avformat_close_input(&audio_context_);

    int error = avformat_open_input(&audio_context_, s.toStdString().c_str(),
            NULL, NULL);
    if (error < 0) {
        qDebug() << "Could not open input file." << s;
        audio_context_ = NULL;
        return;
    }
    if (avformat_find_stream_info(audio_context_, NULL) < 0) {
        qDebug() << "Could not find file info.";
        avformat_close_input(&audio_context_);
        return;
    }
    if (audio_context_->nb_streams != 1) {
        qDebug() << "Expected one audio input stream, but found."
                << audio_context_->nb_streams;
        avformat_close_input(&audio_context_);
        return;
    }

    codec_ = avcodec_find_decoder(audio_context_->streams[0]->codec->codec_id);
    if (!codec_) {
            qDebug() << "Could not find input codec_.";
            avformat_close_input(&audio_context_);
            return;
        }

    error = avcodec_open2(audio_context_->streams[0]->codec,
            codec_, NULL);
    if (error < 0) {
        qDebug() << "Could not open input codec_.";
        avformat_close_input(&audio_context_);
        return;
    }

    codec_context_ = audio_context_->streams[0]->codec;
    qDebug() << "Opened" << codec_->name << "codec_.";
    setupResampler();
}

void AudioPlayer::start()
{
    if (readingFile_ || audio_context_ == NULL)
        return;

    // Thread wasn't shutdown correctly.
    if (!readingFile_ && fileRead_.joinable())
        fileRead_.join();

    qDebug() << "Start";
    readingFile_ = true;
    fileRead_ = std::thread(&AudioPlayer::readAudioFile, this);
}

void AudioPlayer::stop()
{
    qDebug() << "Stop";

    readingFile_ = false;

    if (fileRead_.joinable()) {
        fileRead_.join();
        qDebug() << "Joined";
    }
}

void AudioPlayer::setupResampler()
{
    if (!(resample_context_ = avresample_alloc_context())) {
        fprintf(stderr, "Could not allocate resample context\n");
        return;
    }
    av_opt_set_int(resample_context_, "in_channel_layout",
                   av_get_default_channel_layout(codec_context_->channels), 0);
    av_opt_set_int(resample_context_, "out_channel_layout",
                   av_get_default_channel_layout(outputFormat_.channels), 0);
    av_opt_set_int(resample_context_, "in_sample_rate",
                   codec_context_->sample_rate, 0);
    av_opt_set_int(resample_context_, "out_sample_rate",
                   outputFormat_.rate, 0);
    av_opt_set_int(resample_context_, "in_sample_fmt",
                   codec_context_->sample_fmt, 0);
    av_opt_set_int(resample_context_, "out_sample_fmt",
                   AV_SAMPLE_FMT_S16, 0);
    if (avresample_open(resample_context_) < 0) {
        qDebug() << "Could not open resample context.";
        avresample_free(&resample_context_);
        return;
    }
}

void AudioPlayer::readAudioFile()
{
    // Start at file beginning.
    avformat_seek_file(audio_context_, 0, 0, 0, 0, 0);

    uint8_t *output;
    int out_linesize;
    int out_samples;
    int64_t out_sample_fmt;
    av_opt_get_int(resample_context_, "out_sample_fmt", 0, &out_sample_fmt);

    AVPacket pkt = { 0 };
    av_init_packet(&pkt);
    AVFrame* frm = av_frame_alloc();

    while (readingFile_) {
        int gotFrame = 0;

        readingFile_ = !av_read_frame(audio_context_, &pkt);
        int len = avcodec_decode_audio4(codec_context_, frm, &gotFrame,
                &pkt);

        if (gotFrame) {
            out_samples = avresample_get_out_samples(resample_context_, frm->nb_samples);
            av_samples_alloc(&output, &out_linesize, outputFormat_.channels,
                             out_samples, (AVSampleFormat)out_sample_fmt, 0);
            out_samples = avresample_convert(resample_context_, &output,
                    out_linesize, out_samples, frm->extended_data,
                    frm->linesize[0], frm->nb_samples);

            int ret = avresample_available(resample_context_);
            if (ret)
                fprintf(stderr, "%d converted samples left over\n", ret);

//            printf("Finished reading Frame len : %d , nb_samples:%d buffer_size:%d line size: %d \n",
//                   len,frm->nb_samples,pkt.size,
//                   frm->linesize[0]);
            ao_play(outputDevice_, (char*)output, out_samples*4);
        }
        free(output);
    }

    int out_delay = avresample_get_delay(resample_context_);
    while (out_delay) {
        fprintf(stderr, "Flushed %d delayed resampler samples.\n", out_delay);
        out_samples = avresample_get_out_samples(resample_context_, out_delay);
        av_samples_alloc(&output, &out_linesize, outputFormat_.channels,
                         out_delay, (AVSampleFormat)out_sample_fmt, 0);
        out_delay = avresample_convert(resample_context_, &output, out_linesize,
                out_delay, NULL, 0, 0);
        free(output);
    }
    av_frame_free(&frm);
    av_free_packet(&pkt);

    qDebug() << "Thread stopping.";
}
