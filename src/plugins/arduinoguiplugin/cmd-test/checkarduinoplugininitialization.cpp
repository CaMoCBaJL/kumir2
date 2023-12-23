#include "checkarduinoplugininitialization.h"
#include "ui_checkarduinoplugininitialization.h"
#include "constants.h"

CheckArduinoPluginInitialization::CheckArduinoPluginInitialization(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CheckArduinoPluginInitialization)
{
    ui->setupUi(this);

    ui->arduinoCliInstallation->setLabelText("Проверка установки arduino-cli...");
    ui->arduinoCliAvailability->setLabelText("Проверка доступности arduino-cli...");
    ui->boardsConnectivity->setLabelText("Поиск подключенных плат...");
    this->setPalette(QPalette(Qt::white));
    initCheck();
}

void CheckArduinoPluginInitialization::initCheck(){
    ui->arduinoCliInstallation->setState(ArduinoPlugin::Error);
    ui->arduinoCliAvailability->setState(ArduinoPlugin::Success);
    ui->boardsConnectivity->setState(ArduinoPlugin::Warning);
}

ArduinoPluginCheckState CheckArduinoPluginInitialization::changeState(ArduinoPluginCheckState currentState){

}

CheckArduinoPluginInitialization::~CheckArduinoPluginInitialization()
{
    delete ui;
}
