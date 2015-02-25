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
    bool toggle = false;
    while (readingFile) {
        AVPacket pkt = { 0 };
        av_init_packet(&pkt);
        int gotFrame = 0;
        AVFrame* frm = av_frame_alloc();

        readingFile = !av_read_frame(audio_context, &pkt);
        int len = avcodec_decode_audio4(codec_context, frm, &gotFrame,
                &pkt);

        uint8_t **converted_samples;
//        int newNbSampes = ((float)outputFormat.rate / codec_context->sample_rate) *
//                frm->nb_samples;

        int out_samples = avresample_get_out_samples(resample_context, frm->nb_samples);
        // Little hack because
//        toggle = !toggle;
//        if (toggle) {
//            newNbSampes += 1;
//        }

        init_converted_samples(&converted_samples, out_samples);
        convert_samples(frm->extended_data, converted_samples, frm->nb_samples,
                out_samples);

        len = sizeof(converted_samples);

        if (gotFrame) {
//            printf("Finished reading Frame len : %d , nb_samples:%d buffer_size:%d line size: %d \n",
//                   len,frm->nb_samples,pkt.size,
//                   frm->linesize[0]);
            ao_play(outputDevice, (char*)*converted_samples,
                    len);
        }
        av_frame_free(&frm);
        av_free_packet(&pkt);
    }

    qDebug() << "Thread stopping.";
}

// Blatantly copied from
// https://libav.org/doxygen/master/transcode_aac_8c-example.html
// because it does exactly what we need.
int AudioPlayer::init_converted_samples(uint8_t ***converted_input_samples,
                                  int frame_size)
{
    int error;
    if (!(*converted_input_samples = (uint8_t**)calloc(codec_context->channels,
                                            sizeof(**converted_input_samples)))) {
        fprintf(stderr, "Could not allocate converted input sample pointers\n");
        return AVERROR(ENOMEM);
    }
    if ((error = av_samples_alloc(*converted_input_samples, NULL,
                                  codec_context->channels,
                                  frame_size,
                                  codec_context->sample_fmt, 0)) < 0) {
        fprintf(stderr,
                "Could not allocate converted input samples (error '%x')\n",
                error*-1);
        av_freep(&(*converted_input_samples)[0]);
        free(*converted_input_samples);
        return error;
    }
    return 0;
}

int AudioPlayer::convert_samples(uint8_t **input_data,
                           uint8_t **converted_data, const int inframe_size,
                           const int outframe_size)
{
    int error;
    if ((error = avresample_convert(resample_context, converted_data, 0,
                                    outframe_size, input_data, 0, inframe_size)) < 0) {
        fprintf(stderr, "Could not convert input samples (error '%x')\n",
                error*-1);
        return error;
    }
    int ret;
    if ((ret = avresample_available(resample_context))) {
        fprintf(stderr, "%d converted samples left over\n", ret);
        return AVERROR_EXIT;
    }
    return 0;
}
