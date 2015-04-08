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

/* Initialize libAV and libAO. Set the physical device's parameters. */
AudioPlayer::AudioPlayer() :
    readingFile_(false)
{
    // LibAV initialisation, do not forget this.
    av_register_all();

    // libAO initialisation, http://xiph.org/ao/doc/ao_example.c
    ao_initialize();

    // Ask kindly for the default OS playback device and get
    // an info struct from it. We will use it to set the
    // preferred byte format.
    int default_driver = ao_default_driver_id();
    ao_info* info = ao_driver_info(default_driver);

    // Prepare the device output format. Note that
    // this has nothing to do with the input audio file.
    memset(&outputFormat_, 0, sizeof(outputFormat_));

    outputFormat_.bits = 16; // Could be 24 bits.
    outputFormat_.channels = 2; // Stereo
    outputFormat_.rate = sample_rate; // We set 48KHz
    outputFormat_.byte_format = info->preferred_byte_format; // Endianness
    outputFormat_.matrix = 0; // Channels to speaker setup. Unused.

    // Start the output device with our desired format.
    // If all goes well we will be able to playback audio.
    outputDevice_ = ao_open_live(default_driver, &outputFormat_,
            NULL /* no options */);
    if (outputDevice_ == NULL) {
        fprintf(stderr, "Error opening device.\n");
        return;
    }
}

/* Cleanup */
AudioPlayer::~AudioPlayer()
{
    // Don't forget to join the thread.
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

/* Try to load and check if the audio file is supported.
Audio headers are sometimes damaged, misinformed or non-existant,
so we will try to read a bit of the file to figure out the codec.
LibAV is awesome like that.

Example https://libav.org/doxygen/master/transcode_aac_8c-example.html */
void AudioPlayer::setFile(QString &s)
{
    // Make sure the audio context is free.
    if (audio_context_ != NULL)
        avformat_close_input(&audio_context_);

    // Try and open up the file (check if file exists).
    int error = avformat_open_input(&audio_context_, s.toStdString().c_str(),
            NULL, NULL);
    if (error < 0) {
        qDebug() << "Could not open input file." << s;
        audio_context_ = NULL;
        return;
    }

    // Read packets of a media file to get stream information.
    if (avformat_find_stream_info(audio_context_, NULL) < 0) {
        qDebug() << "Could not find file info.";
        avformat_close_input(&audio_context_);
        return;
    }

    // We need an audio stream. Note that a stream can contain
    // many channels. Some formats support more streams.
    if (audio_context_->nb_streams != 1) {
        qDebug() << "Expected one audio input stream, but found"
                << audio_context_->nb_streams;
        avformat_close_input(&audio_context_);
        return;
    }

    // If all went well, we have an audio file and have identified a
    // codec. Lets try and find the required decoder.
    codec_ = avcodec_find_decoder(audio_context_->streams[0]->codec->codec_id);
    if (!codec_) {
            qDebug() << "Could not find input codec.";
            avformat_close_input(&audio_context_);
            return;
        }

    // And lets open the decoder.
    error = avcodec_open2(audio_context_->streams[0]->codec,
            codec_, NULL);
    if (error < 0) {
        qDebug() << "Could not open input codec_.";
        avformat_close_input(&audio_context_);
        return;
    }

    // All went well, we store the codec information for later
    // use in the resampler setup.
    codec_context_ = audio_context_->streams[0]->codec;
    qDebug() << "Opened" << codec_->name << "codec_.";
    setupResampler();
}

/* Set up a resampler to convert our file to
the desired device playback. We set all the parameters we
used when initialising libAO in our constructor. */
void AudioPlayer::setupResampler()
{
    // Prepare resampler.
    if (!(resample_context_ = avresample_alloc_context())) {
        fprintf(stderr, "Could not allocate resample context\n");
        return;
    }

    // The file channels.
    av_opt_set_int(resample_context_, "in_channel_layout",
                   av_get_default_channel_layout(codec_context_->channels), 0);
    // The device channels.
    av_opt_set_int(resample_context_, "out_channel_layout",
                   av_get_default_channel_layout(outputFormat_.channels), 0);
    // The file sample rate.
    av_opt_set_int(resample_context_, "in_sample_rate",
                   codec_context_->sample_rate, 0);
    // The device sample rate.
    av_opt_set_int(resample_context_, "out_sample_rate",
                   outputFormat_.rate, 0);
    // The file bit-dpeth.
    av_opt_set_int(resample_context_, "in_sample_fmt",
                   codec_context_->sample_fmt, 0);

    // The device bit-depth.
    // FIXME: If you change the device bit-depth, you have to change
    // this value manually.
    av_opt_set_int(resample_context_, "out_sample_fmt",
                   AV_SAMPLE_FMT_S16, 0);

    // And now open the resampler. Hopefully all went well.
    if (avresample_open(resample_context_) < 0) {
        qDebug() << "Could not open resample context.";
        avresample_free(&resample_context_);
        return;
    }
}

/* If all of the above went according to plan, we launch a
thread to read and playback the file. */
void AudioPlayer::start()
{
    // We are currently reading a file, or something bad happened.
    if (readingFile_ || audio_context_ == NULL)
        return;

    // Thread wasn't shutdown correctly.
    if (!readingFile_ && fileRead_.joinable())
        fileRead_.join();

    qDebug() << "Start";
    readingFile_ = true;
    // Launch the thread.
    fileRead_ = std::thread(&AudioPlayer::readAudioFile, this);
}

/* Stop playback, join the thread. */
void AudioPlayer::stop()
{
    qDebug() << "Stop";
    readingFile_ = false;

    if (fileRead_.joinable()) {
        fileRead_.join();
        qDebug() << "Joined";
    }
}

/* Actual audio decoding, resampling and reading. Once a frame is
ready, copy the converted values to libAO's playback buffer. Since
you have access to the raw data, you could hook up a callback and
modify/analyse the audio data here. */
void AudioPlayer::readAudioFile()
{
    // Start at the file beginning.
    avformat_seek_file(audio_context_, 0, 0, 0, 0, 0);

    uint8_t *output; // This is the audio data buffer.
    int out_linesize; // Used internally by libAV.
    int out_samples; // How many samples we will play, AFTER resampling.
    int64_t out_sample_fmt; // Bit-depth.

    // We need to use this "getter" for the output sample format.
    av_opt_get_int(resample_context_, "out_sample_fmt", 0, &out_sample_fmt);

    // Initialize all packet values to 0.
    AVPacket pkt = { 0 };
    av_init_packet(&pkt);
    AVFrame* frm = av_frame_alloc();

    // Loop till file is read.
    while (readingFile_) {
        int gotFrame = 0;

        // Fill packets with data. If no data is read, we are done.
        readingFile_ = !av_read_frame(audio_context_, &pkt);

        // Len was used for debugging purpose. Decode the audio data.
        // Put that in AVFrame.
        int len = avcodec_decode_audio4(codec_context_, frm, &gotFrame,
                &pkt);

        // We can play audio.
        if (gotFrame) {

            // Calculate how many samples we will have after resampling.
            out_samples = avresample_get_out_samples(resample_context_,
                    frm->nb_samples);

            // Allocate our output buffer.
            av_samples_alloc(&output, &out_linesize, outputFormat_.channels,
                    out_samples, (AVSampleFormat)out_sample_fmt, 0);

            // Resample the audio data and store it in our output buffer.
            out_samples = avresample_convert(resample_context_, &output,
                    out_linesize, out_samples, frm->extended_data,
                    frm->linesize[0], frm->nb_samples);

            // This is why we store out_samples again, some issues may
            // have occured.
            int ret = avresample_available(resample_context_);
            if (ret)
                fprintf(stderr, "%d converted samples left over\n", ret);

            // DEBUG
//            printf("Finished reading Frame len : %d , nb_samples:%d buffer_size:%d line size: %d \n",
//                   len,frm->nb_samples,pkt.size,
//                   frm->linesize[0]);

            // Finally, play the raw data using libAO.
            ao_play(outputDevice_, (char*)output, out_samples*4);
        }
        free(output);
    }

    // Because of delays, there may be some leftover resample data.
    int out_delay = avresample_get_delay(resample_context_);

    // Clean it.
    while (out_delay) {
        fprintf(stderr, "Flushed %d delayed resampler samples.\n", out_delay);

        // You get rid of the remaining data by "resampling" it with a NULL
        // input.
        out_samples = avresample_get_out_samples(resample_context_, out_delay);
        av_samples_alloc(&output, &out_linesize, outputFormat_.channels,
                out_delay, (AVSampleFormat)out_sample_fmt, 0);
        out_delay = avresample_convert(resample_context_, &output, out_linesize,
                out_delay, NULL, 0, 0);
        free(output);
    }

    // Cleanup.
    av_frame_free(&frm);
    av_free_packet(&pkt);

    qDebug() << "Thread stopping.";
}
