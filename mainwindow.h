#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <string>
#include <vector>

#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include <lilv/lilv.h>

#include "LV2_Classes.h"
#include "audioplayer.h"

const double sample_rate = 44100.0;
const int buf_size = 512;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void printRequiredFeatures(const LilvPlugin* plug);
    void messageUser(QString s);

private slots:
    void on_plugins_activated(int index);
    void on_actionOpen_triggered();
    void on_play_clicked();
    void on_stop_clicked();
    void buf_callback(const QAudioBuffer& b);

private:
    Ui::MainWindow *ui;
    LilvWorld* world;
    const LilvPlugins* plugins;
    std::vector<LilvInstance*> activePlugins;

    float send[buf_size];
    float recv[buf_size];

    AudioPlayer mainOut;
};

#endif // MAINWINDOW_H
