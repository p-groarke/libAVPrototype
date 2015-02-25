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
    readingFile(false)
{
    // libAV
    av_register_all();

    // libao http://xiph.org/ao/doc/ao_example.c
    ao_initialize();
    int default_driver = ao_default_driver_id();

    ao_info* info = ao_driver_info(ao_default_driver_id());


    memset(&outputFormat, 0, sizeof(outputFormat));
    outputFormat.bits = 16;
    outputFormat.channels = 2;
    outputFormat.rate = 48000;
    outputFormat.byte_format = info->preferred_byte_format;
    outputFormat.matrix = 0;

    outputDevice = ao_open_live(default_driver, &outputFormat,
            NULL /* no options */);
    if (outputDevice == NULL) {
        fprintf(stderr, "Error opening device.\n");
        return;
    }

}

AudioPlayer::~AudioPlayer()
{
    readingFile = false;
    if (fileRead.joinable()) {
        fileRead.join();
        qDebug() << "Joined";
    }

    if (audio_context != NULL)
        avformat_close_input(&audio_context);

    if (resample_context != NULL)
        avresample_close(resample_context);

    ao_close(outputDevice);
    ao_shutdown();
}

// Example https://libav.org/doxygen/master/transcode_aac_8c-example.html
void AudioPlayer::setFile(QString &s)
{
    int error = avformat_open_input(&audio_context, s.toStdString().c_str(),
            NULL, NULL);
    if (error < 0) {
        fprintf(stderr, "Could not open input file '%s' (error '%x')\n",
                s.toStdString().c_str(), error*-1);
        audio_context = NULL;
        return;
    }
    if (avformat_find_stream_info(audio_context, NULL) < 0) {
        fprintf(stderr, "Could not find file info\n");
        avformat_close_input(&audio_context);
        return;
    }
    if (audio_context->nb_streams != 1) {
        fprintf(stderr, "Expected one audio input stream, but found %d\n",
                audio_context->nb_streams);
        avformat_close_input(&audio_context);
        return;
    }

    codec = avcodec_find_decoder(audio_context->streams[0]->codec->codec_id);
    if (!codec) {
            fprintf(stderr, "Could not find input codec\n");
            avformat_close_input(&audio_context);
            return;
        }

    error = avcodec_open2(audio_context->streams[0]->codec,
            codec, NULL);
    if (error < 0) {
        fprintf(stderr, "Could not open input codec (error '%x')\n",
                error*-1);
        avformat_close_input(&audio_context);
        return;
    }

    codec_context = audio_context->streams[0]->codec;
    qDebug() << "Opened" << codec->name << "codec.";
    setupResampler();
}

void AudioPlayer::start()
{
    if (readingFile || audio_context == NULL)
        return;

    // Thread wasn't shutdown correctly.
    if (!readingFile && fileRead.joinable())
        fileRead.join();

    qDebug() << "Start";
    readingFile = true;
    fileRead = std::thread(&AudioPlayer::readAudioFile, this);
}

void AudioPlayer::stop()
{
    qDebug() << "Stop";

    readingFile = false;

    if (fileRead.joinable()) {
        fileRead.join();
        qDebug() << "Joined";
    }
}

void AudioPlayer::setupResampler()
{
    if (!(resample_context = avresample_alloc_context())) {
        fprintf(stderr, "Could not allocate resample context\n");
        return;
    }
    av_opt_set_int(resample_context, "in_channel_layout",
                   av_get_default_channel_layout(codec_context->channels), 0);
    av_opt_set_int(resample_context, "out_channel_layout",
                   av_get_default_channel_layout(outputFormat.channels), 0);
    av_opt_set_int(resample_context, "in_sample_rate",
                   codec_context->sample_rate, 0);
    av_opt_set_int(resample_context, "out_sample_rate",
                   outputFormat.rate, 0);
    av_opt_set_int(resample_context, "in_sample_fmt",
                   codec_context->sample_fmt, 0);
    av_opt_set_int(resample_context, "out_sample_fmt",
                   AV_SAMPLE_FMT_S16, 0);
    if (avresample_open(resample_context) < 0) {
        fprintf(stderr, "Could not open resample context\n");
        avresample_free(&resample_context);
        return;
    }
}

void AudioPlayer::readAudioFile()
{
    // Start at file beginning.
    avformat_seek_file(audio_context, 0, 0, 0, 0, 0);

    uint8_t *output;
    int out_linesize;
    int out_samples;
    int64_t out_sample_fmt;
    av_opt_get_int(resample_context, "out_sample_fmt", 0, &out_sample_fmt);

    AVPacket pkt = { 0 };
    av_init_packet(&pkt);
    AVFrame* frm = av_frame_alloc();

    while (readingFile) {
        int gotFrame = 0;

        readingFile = !av_read_frame(audio_context, &pkt);
        int len = avcodec_decode_audio4(codec_context, frm, &gotFrame,
                &pkt);

        if (gotFrame) {
            out_samples = avresample_get_out_samples(resample_context, frm->nb_samples);
            av_samples_alloc(&output, &out_linesize, outputFormat.channels,
                             out_samples, (AVSampleFormat)out_sample_fmt, 0);
            out_samples = avresample_convert(resample_context, &output,
                    out_linesize, out_samples, frm->extended_data,
                    frm->linesize[0], frm->nb_samples);

            int ret = avresample_available(resample_context);
            if (ret)
                fprintf(stderr, "%d converted samples left over\n", ret);

//            printf("Finished reading Frame len : %d , nb_samples:%d buffer_size:%d line size: %d \n",
//                   len,frm->nb_samples,pkt.size,
//                   frm->linesize[0]);
            ao_play(outputDevice, (char*)output, out_samples*4);
        }
        free(output);
    }

    int out_delay = avresample_get_delay(resample_context);
    while (out_delay) {
        fprintf(stderr, "Flushed %d delayed resampler samples.\n", out_delay);
        out_samples = avresample_get_out_samples(resample_context, out_delay);
        av_samples_alloc(&output, &out_linesize, outputFormat.channels,
                         out_delay, (AVSampleFormat)out_sample_fmt, 0);
        out_delay = avresample_convert(resample_context, &output, out_linesize,
                out_delay, NULL, 0, 0);
        free(output);
    }
    av_frame_free(&frm);
    av_free_packet(&pkt);

    qDebug() << "Thread stopping.";
}
