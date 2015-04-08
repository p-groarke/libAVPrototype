/*
 *
 * mainwindow.cpp
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
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QMessageBox>
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

/* Open file dialogue. */
void MainWindow::on_actionOpen_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open File"),
        getenv("HOME"),
//        tr("Wave File (*.wav)"));
        tr("All (*.*)"));

    if (!fileName.isEmpty()) {
        ui->filename->setText(QFileInfo(fileName).fileName());
        qDebug() << fileName;
        mainOut.setFile(fileName);
    }
}

/* Start audio playback. */
void MainWindow::on_play_clicked()
{
    mainOut.start();
}

/* Stop audio playback. */
void MainWindow::on_stop_clicked()
{
    mainOut.stop();
}
