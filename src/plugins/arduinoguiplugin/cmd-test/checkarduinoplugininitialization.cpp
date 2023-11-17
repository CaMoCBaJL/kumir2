#include "checkarduinoplugininitialization.h"
#include "ui_checkarduinoplugininitialization.h"

CheckArduinoPluginInitialization::CheckArduinoPluginInitialization(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CheckArduinoPluginInitialization)
{
    ui->setupUi(this);
    initCheck();
}

void CheckArduinoPluginInitialization::initCheck(){

}

ArduinoPluginCheckState CheckArduinoPluginInitialization::changeState(ArduinoPluginCheckState currentState){

}

CheckArduinoPluginInitialization::~CheckArduinoPluginInitialization()
{
    delete ui;
}
