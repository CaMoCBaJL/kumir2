#ifndef CHECKARDUINOPLUGININITIALIZATION_H
#define CHECKARDUINOPLUGININITIALIZATION_H

#include <QDialog>
#include "check_steps.h"
using ArduinoPlugin::ArduinoPluginCheckState;

namespace Ui {
class CheckArduinoPluginInitialization;
}

class CheckArduinoPluginInitialization : public QDialog
{
    Q_OBJECT

public:
    explicit CheckArduinoPluginInitialization(QWidget *parent = nullptr);
    ~CheckArduinoPluginInitialization();

private:
    Ui::CheckArduinoPluginInitialization *ui;

    void initCheck();
    ArduinoPluginCheckState changeState(ArduinoPluginCheckState currentState);
};

#endif // CHECKARDUINOPLUGININITIALIZATION_H
