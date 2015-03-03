#pragma once

#include <string>

#include <lv2/lv2plug.in/ns/extensions/ui/ui.h>
#include <lilv/lilv.h>
#include <suil/suil.h>

#include "plugin.h"
#include "globals.h"
#include "ringbuffer.h"

class QDialog;

class LVPlugin : public Plugin
{
public:
    LVPlugin(LilvWorld* w, const LilvPlugin* me);
    ~LVPlugin();

    const LilvPlugin* getPlug();
    // suil implementations

    Ringbuffer uiEventsBuffer_;

private:
    void setupUI();
    void messageUser(QString s);
    void printRequiredFeatures();

    static void ui_write(SuilController controller, uint32_t port_index,
            uint32_t buffer_size, uint32_t protocol, const void* buffer);
    static uint32_t ui_port_index(SuilController, const char* symbol);

    LilvWorld* world_;
    // Plugin
    const LilvPlugin* myPlug_;
    LilvInstance* myInstance_;
    std::string myPlugURI_;
    // UI
    LilvUI* myUI_;
    LilvUIs* myUIs_;
    SuilHost* mySuilHost_;
    SuilInstance* mySuilInstance_;
    QDialog* test;



};

