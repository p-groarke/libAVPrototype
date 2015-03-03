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

#include "lvplugin.h"

#include <QDebug>
#include <QMessageBox>
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    world = lilv_world_new();

    if (world == NULL)
        fprintf(stderr, "lilv didn't create world\n");

    lilv_world_load_all(world);

    plugins = lilv_world_get_all_plugins(world);
    qDebug() << "Lilv found" << lilv_plugins_size(plugins) << "plugins.";

    LilvIter* it = lilv_plugins_begin(plugins);

    while (!lilv_plugins_is_end(plugins, it)) {
        const LilvPlugin* p = lilv_plugins_get(plugins, it);
        const LilvNode* n = lilv_plugin_get_name(p);
        QString pname(lilv_node_as_string(n));
        n = lilv_plugin_get_uri(p);
        QString uri(lilv_node_as_uri(n));
        ui->plugins->addItem(pname, uri);
        it = lilv_plugins_next(plugins, it);
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_plugins_activated(int index)
{
    std::string t = ui->plugins->itemData(index).toString().toStdString();
    const LilvNode* n = lilv_new_uri(world, t.c_str());
    const LilvPlugin* plug = lilv_plugins_get_by_uri(plugins, n);

    currentPlugin = std::move(std::unique_ptr<LVPlugin>(
            new LVPlugin(world, plug)));
}

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

void MainWindow::on_play_clicked()
{
    mainOut.start();
}

void MainWindow::on_stop_clicked()
{
    mainOut.stop();
}
