/*
 *
 * mainwindow.h
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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <string>
#include <vector>

#include "audioplayer.h"
#include "globals.h"


namespace Ui {
class MainWindow;
}

class LVPlugin;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_actionOpen_triggered();
    void on_play_clicked();
    void on_stop_clicked();

private:
    Ui::MainWindow *ui;
    AudioPlayer mainOut;
};

#endif // MAINWINDOW_H
