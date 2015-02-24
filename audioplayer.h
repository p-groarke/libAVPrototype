#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <QMediaPlayer>
#include <QAudioProbe>
#include <QAudioBuffer>
#include <QAudioOutput>
#include <QAudioDecoder>
#include <QIODevice>
#include <QPointer>
#include <QFile>

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

#include <ao/ao.h>

extern "C" {
#include <libavformat/avformat.h>
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
    void readAudioFile();

    QFile* audioFile;

    AVFormatContext* audio_context = NULL;

    ao_device* outputDevice;
//    QAudioOutput* mainOut;
//    QAudioDecoder* decoder;
//    QMediaPlayer* player;
//    const QIODevice* stream;

    std::atomic<bool> readingFile;
    std::thread fileRead;
};

#endif // AUDIOPLAYER_H
