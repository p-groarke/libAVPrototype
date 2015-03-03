#include "lvplugin.h"
#include "lv2_classes.hpp"

#include <QDebug>
#include <QDialog>
#include <QMessageBox>

LVPlugin::LVPlugin(LilvWorld* w, const LilvPlugin *me) :
    uiEventsBuffer_(sizeof(ControlChange) * 64)
{
    world_ = w;
    myPlug_ = me;
    myPlugURI_ = lilv_node_as_string(lilv_plugin_get_uri(myPlug_));
    printRequiredFeatures();

    myInstance_ = lilv_plugin_instantiate(myPlug_, sample_rate, NULL);
    if (myInstance_ == NULL) {
        messageUser(QString("Couldn't instantiate plugin."));
        return;
    }

    int i = lilv_plugin_get_num_ports(myPlug_) - 1;
    for (i; i >= 0; --i) {
        const LilvPort* port = lilv_plugin_get_port_by_index(myPlug_, i);
        if (port == NULL) {
            messageUser(QString("Couldn't get plugin port."));
            return;
        }

        LV2_Classes c(world_);
        if (lilv_port_is_a(myPlug_, port, c.input_class)) {
            lilv_instance_connect_port(myInstance_, i, inBuffer);
        } else if (lilv_port_is_a(myPlug_, port, c.output_class)) {
            lilv_instance_connect_port(myInstance_, i, outBuffer);
        }
    }

    lilv_instance_activate(myInstance_);
    //lilv_instance_run(myInstance, 512); // TEMPORARY
    setupUI();
}

LVPlugin::~LVPlugin()
{
    //lilv_uis_free(myUIs);
}

const LilvPlugin *LVPlugin::getPlug()
{
    return myPlug_;
}

void LVPlugin::setupUI()
{
//    myUI_ = lilv_plugin_get_u
    myUIs_ = lilv_plugin_get_uis(myPlug_);

    LilvIter* it = lilv_uis_begin(myUIs_);
    while(!lilv_uis_is_end(myUIs_, it)) {
        qDebug() << lilv_node_as_string(
                lilv_ui_get_uri(lilv_uis_get(myUIs_, it)));
        it = lilv_uis_next(myUIs_, it);
    }

    mySuilHost_ = suil_host_new(LVPlugin::ui_write,
            LVPlugin::ui_port_index, NULL, NULL);

//    mySuilInstance_ = suil_instance_new(mySuilHost_, this,
//            "http://lv2plug.in/ns/extensions/ui#Qt5UI",
//            myPlugURI_, )

    test = new QDialog();
    test->show();


}

void LVPlugin::printRequiredFeatures()
{
    qDebug() << "Plugin required features:";
    LilvNodes* feats = lilv_plugin_get_required_features(myPlug_);
    LilvIter* it = lilv_nodes_begin(feats);
    while(!lilv_nodes_is_end(feats, it)) {
        qDebug() << lilv_node_as_string(lilv_nodes_get(feats, it));
        it = lilv_nodes_next(feats, it);
    }
    qDebug() << "\n";
}

void LVPlugin::messageUser(QString s)
{
    QMessageBox messageBox;
    messageBox.critical(0,"Aaaaaa", s);
    messageBox.setFixedSize(500,200);
}

void LVPlugin::ui_write(SuilController controller, uint32_t port_index,
        uint32_t buffer_size, uint32_t protocol, const void* buffer)
{

    LVPlugin* const plug = (LVPlugin*)controller;

    if (protocol != 0) {
        qDebug() << "UI protocol unsupported";
        return;
    }

    char buf[sizeof(ControlChange) + buffer_size];
    ControlChange* event = (ControlChange*)buf;
    event->index    = port_index;
    event->protocol = protocol;
    event->size     = buffer_size;
    memcpy(event->body, buffer, buffer_size);

    // Write to ringbuffer
    plug->uiEventsBuffer_.write(buf, sizeof(buf));

}

uint32_t LVPlugin::ui_port_index(SuilController controller, const char* symbol)
{
    LVPlugin* const plug = (LVPlugin*)controller;

    int i = lilv_plugin_get_num_ports(plug->getPlug()) - 1;
    for (i; i >= 0; --i) {
        const LilvPort* port = lilv_plugin_get_port_by_index(
                plug->getPlug(), i);
        const LilvNode* portSymbol = lilv_port_get_symbol(
                plug->getPlug(), port);

        if (!strcmp(lilv_node_as_string(portSymbol), symbol)){
            return i;
        }
    }

    return LV2UI_INVALID_PORT_INDEX;

}
