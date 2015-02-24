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

    world = lilv_world_new();

    if (world == NULL)
        fprintf(stderr, "lilv didn't create world\n");

    lilv_world_load_all(world);

    plugins = lilv_world_get_all_plugins(world);
    qDebug() << "Lilv found " << lilv_plugins_size(plugins) << "plugins.";

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
    for (auto x : activePlugins)
        lilv_instance_free(x);

    delete ui;
}

void MainWindow::printRequiredFeatures(const LilvPlugin *plug)
{
    qDebug() << "Plugin required features:";
    LilvNodes* feats = lilv_plugin_get_required_features(plug);
    LilvIter* it = lilv_nodes_begin(feats);
    while(!lilv_nodes_is_end(feats, it)) {
        qDebug() << lilv_node_as_string(lilv_nodes_get(feats, it));
        it = lilv_nodes_next(feats, it);
    }
    qDebug() << "\n";
}

void MainWindow::messageUser(QString s)
{
    QMessageBox messageBox;
    messageBox.critical(0,"Aaaaaa", s);
    messageBox.setFixedSize(500,200);
}

void MainWindow::on_plugins_activated(int index)
{
    std::string t = ui->plugins->itemData(index).toString().toStdString();
    const LilvNode* n = lilv_new_uri(world, t.c_str());
    const LilvPlugin* plug = lilv_plugins_get_by_uri(plugins, n);

    printRequiredFeatures(plug);

    LilvInstance* p = lilv_plugin_instantiate(plug, sample_rate, NULL);
    if (p == NULL) {
        messageUser(QString("Couldn't instantiate plugin."));
        return;
    }

    int i = lilv_plugin_get_num_ports(plug) - 1;
    for (i; i >= 0; --i) {
        const LilvPort* port = lilv_plugin_get_port_by_index(plug, i);
        if (port == NULL) {
            messageUser(QString("Couldn't get plugin port."));
            return;
        }

        LV2_Classes c(world);
        if (lilv_port_is_a(plug, port, c.input_class)) {
            lilv_instance_connect_port(p, i, send);
        } else if (lilv_port_is_a(plug, port, c.output_class)) {
            lilv_instance_connect_port(p, i, recv);
        }
    }

    lilv_instance_activate(p);

    activePlugins.push_back(p);

    lilv_instance_run(p, 512);

}

void MainWindow::on_actionOpen_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open File"),
        getenv("HOME"),
        tr("Wave File (*.wav)"));

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

void MainWindow::buf_callback(const QAudioBuffer &b)
{
    qDebug() << "buffer callback";
}
