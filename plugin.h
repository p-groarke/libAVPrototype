#ifndef PLUGIN_H
#define PLUGIN_H

#include <QObject>

class Plugin : public QObject
{
    Q_OBJECT
public:
    explicit Plugin(QObject *parent = 0);
    ~Plugin();

protected:
    uint8_t inBuffer[512];
    uint8_t outBuffer[512];
signals:

public slots:
};

#endif // PLUGIN_H
