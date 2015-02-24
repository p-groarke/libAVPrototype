#include "audioplayer.h"

AudioPlayer::AudioPlayer() :
    readingFile(false)
{
    av_register_all();

//    QAudioFormat format;
//    format.setSampleRate(48000);
//    format.setChannelCount(2);
//    format.setSampleSize(16);
//    format.setCodec("audio/pcm");
//    format.setSampleType(QAudioFormat::UnSignedInt);

//    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
//    if (!info.isFormatSupported(format)) {
//        qWarning() << "Audio format not supported by backend.";
//        return;
//    }

//    decoder = new QAudioDecoder();
//    decoder->setAudioFormat(format);
//    connect(decoder, SIGNAL(bufferReady()),
//            this, SLOT(readBuffer()));

//    mainOut = new QAudioOutput(format);
//    connect(mainOut, SIGNAL(stateChanged(QAudio::State)),
//            this, SLOT(handleStateChanged(QAudio::State)));

//    player = new QMediaPlayer();
}

AudioPlayer::~AudioPlayer()
{
    readingFile = false;
    if (fileRead.joinable()) {
        fileRead.join();
        qDebug() << "Joined";
    }
}

void AudioPlayer::setFile(QString &s)
{
//    const char* url = "/Volumes/User/pgroarke/Music/test.wav";
    int error = avformat_open_input(&audio_context, s.toStdString().c_str(),
            NULL, NULL);

    if (error < 0) {
        fprintf(stderr, "Could not open input file '%s' (error '%x')\n",
                s.toStdString().c_str(), error*-1);
        audio_context = NULL;
        return;
    }

//    player->setMedia(QUrl::fromLocalFile(s));

//    stream = player->mediaStream();
//    connect(stream, SIGNAL(readyRead()), this, SLOT(readBuffer()));

//    decoder->setSourceFilename(s);
//    audioFile = new QFile(fileName);
//    audioFile->open(QIODevice::ReadOnly);

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

//    player->play();
//    decoder->start();
}

void AudioPlayer::stop()
{
    qDebug() << "Stop";

    readingFile = false;

    if (fileRead.joinable()) {
        fileRead.join();
        qDebug() << "Joined";
    }


//    fileRead.join();
//    player->stop();
}

void AudioPlayer::readAudioFile()
{
    AVPacket pkt = { 0 };
    av_init_packet(&pkt);
    AVFrame* frm = av_frame_alloc();

    while (readingFile) {
        readingFile = !av_read_frame(audio_context, &pkt);
        qDebug() << "fileThread " << pkt.data;
    }

    qDebug() << "Thread stopping.";
//    av_frame_free(&frm);
//    av_free_packet(&pkt);ß
}
