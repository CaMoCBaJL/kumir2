/*
This file is generated, but you can safely change it
until you run "gen_actor_source.py" with "--project" flag.

Generated file is just a skeleton for module contents.
You should change it corresponding to functionality.
*/

// Self include
#include "arduinomodule.h"

// Kumir includes
#include <kumir2-libs/extensionsystem/kplugin.h>

// Qt includes
#include <QtCore>
#if QT_VERSION >= 0x050000
#   include <QtWidgets>
#else
#   include <QtGui>
#endif

namespace ActorArduino {

    void ArduinoModule::setupSettings(){
    mySettings()->setValue(flashTypeSettingLabel, QVariant(false));
    mySettings()->setValue(robotTypeSettingLabel, QVariant(QString("")));
}

QVariant ArduinoModule::getFlashType(){
    return mySettings()->value(flashTypeSettingLabel);
}

void ArduinoModule::changeFlashType(bool value){
    auto flashType = getFlashType();
    if (flashType != value) {
        mySettings()->setValue(flashTypeSettingLabel, QVariant(value));
    }
}

void ArduinoModule::createFlashTypeListMenu(QMenu* parent){
    QActionGroup* flashTypeMenuActions = new QActionGroup(parent);

    auto cicleAction = parent->addAction(flashTypeMenuItemLabels[0]);
    cicleAction->setCheckable(true);
    flashTypeMenuActions->addAction(cicleAction);
    connect(cicleAction, &QAction::triggered, this, [=](){ changeFlashType(true);});

    auto setupAction = parent->addAction(flashTypeMenuItemLabels[1]);
    setupAction->setCheckable(true);
    flashTypeMenuActions->addAction(setupAction);
    connect(setupAction, &QAction::triggered, this, [=](){ changeFlashType(false);});

    auto flashType = getFlashType();

    if (flashType.isNull() || !flashType.toBool()){
        setupAction->setChecked(true);
    }
    else {
        cicleAction->setChecked(true);
    }
}

//TODO: add RobotConfig class: {title: string, pathToFile: string[]}
//and change use of QStringList to RobotConfig
QStringList ArduinoModule::getRobotConfigurationList(){
    return mySettings()->value(robotListSettingLabel).toStringList();
}

void ArduinoModule::chooseRobotType(QString robotConfig){
    mySettings()->setValue(robotTypeSettingLabel, robotConfig);
}

void ArduinoModule::addRobotType(){
    //empty method, add custom logic here
}

void ArduinoModule::createRobotTypeListMenu(QMenu* parent){
    QStringList robotConfigList = getRobotConfigurationList();

    if (robotConfigList.size() > 0) {
        QActionGroup* robotTypeMenuActions = new QActionGroup(parent);
        auto robotType = mySettings()->value(robotTypeSettingLabel);

        for(int i = 0; i < robotConfigList.size(); i++) {
            auto configAction = parent->addAction(robotConfigList.at(i));
            configAction->setCheckable(true);
            connect(configAction, &QAction::triggered, this, [=](){chooseRobotType(robotConfigList.at(i));});

            robotTypeMenuActions->addAction(configAction);

            if (robotConfigList.at(i) == robotType) {
                configAction->setChecked(true);
            }

            parent->addAction(configAction);
        }

        delete robotTypeMenuActions;
    }

    auto addRobotConfigAction = parent->addAction(addRobotTypeMenuItem);
    connect(addRobotConfigAction, &QAction::triggered, this, [=](){addRobotType();});
}

QStringList ArduinoModule::getBoardList(){
    //mock method, add custom logic here
    return QStringList();
}

void ArduinoModule::chooseBoard(QAction *menuItem){
    menuItem->setChecked(true);
}

void ArduinoModule::createBoardListMenu(QMenu* parent){
    QStringList boardList = getBoardList();

    if (boardList.size() > 0) {
        QActionGroup* boardListMenuActions = new QActionGroup(parent);

        for(int i = 0; i < boardList.size(); i++) {
            auto configAction = parent->addAction(boardList.at(i));
            configAction->setCheckable(true);
            connect(configAction, &QAction::triggered, this, [=](){  ;});

            boardListMenuActions->addAction(configAction);
        }

        delete boardListMenuActions;
    }
}

ArduinoModule::ArduinoModule(ExtensionSystem::KPlugin * parent)
    : ArduinoModuleBase(parent)
{
    setupSettings();
    createGui();
}

void ArduinoModule::createGui() {
    createFlashTypeListMenu(m_menuArduino->addMenu(flashTypeListMenu));
    createBoardListMenu(m_menuArduino->addMenu(boardListMenu));
    createRobotTypeListMenu(m_menuArduino->addMenu(robotTypeListMenu));
}

/* public static */ QList<ExtensionSystem::CommandLineParameter> ArduinoModule::acceptableCommandLineParameters()
{
    // See "src/shared/extensionsystem/commandlineparameter.h" for constructor details
    return QList<ExtensionSystem::CommandLineParameter>();
}

/* public slot */ void ArduinoModule::changeGlobalState(ExtensionSystem::GlobalState old, ExtensionSystem::GlobalState current)
{
    // Called when changed kumir state. The states are defined as enum ExtensionSystem::GlobalState:
    /*
    namespace ExtensionSystem {
        enum GlobalState {
            GS_Unlocked, // Edit mode
            GS_Observe, // Observe mode
            GS_Running, // Running mode
            GS_Input,  // User input required
            GS_Pause  // Running paused
        };
    }
    */
    // TODO implement me
    using namespace ExtensionSystem;  // not to write "ExtensionSystem::" each time in this method scope
    Q_UNUSED(old);  // Remove this line on implementation
    Q_UNUSED(current);  // Remove this line on implementation
}

/* public slot */ void ArduinoModule::loadActorData(QIODevice * source)
{
    // Set actor specific data (like environment)
    // The source should be ready-to-read QIODevice like QBuffer or QFile
    Q_UNUSED(source);  // By default do nothing

}

/* public */ QWidget* ArduinoModule::mainWidget() const
{
    // Returns module main view widget, or nullptr if there is no any views
    // NOTE: the method is const and might be called at any time,
    //       so DO NOT create widget here, just return!
    // TODO implement me
    return nullptr;
}

/* public */ QWidget* ArduinoModule::pultWidget() const
{
    // Returns module control view widget, or nullptr if there is no control view
    // NOTE: the method is const and might be called at any time,
    //       so DO NOT create widget here, just return!
    // TODO implement me
    return nullptr;
}

/* public slot */ void ArduinoModule::reloadSettings(ExtensionSystem::SettingsPtr settings, const QStringList & keys)
{
    // Updates setting on module load, workspace change or appliyng settings dialog.
    // If @param keys is empty -- should reload all settings, otherwise load only setting specified by @param keys
    // TODO implement me
    Q_UNUSED(settings);  // Remove this line on implementation
    Q_UNUSED(keys);  // Remove this line on implementation
}

/* public slot */ void ArduinoModule::reset()
{
    // Resets module to initial state before program execution
    // TODO implement me
}

/* public slot */ void ArduinoModule::setAnimationEnabled(bool enabled)
{
    // Sets GUI animation flag on run
    // NOTE this method just setups a flag and might be called anytime, even module not needed
    // TODO implement me
    Q_UNUSED(enabled);  // Remove this line on implementation
}

/* public slot */ void ArduinoModule::terminateEvaluation()
{
    // Called on program interrupt to ask long-running module's methods
    // to stop working
    // TODO implement me
}

/* public slot */ bool ArduinoModule::runDigitalRead(const int pin)
{
    /* алг лог цифрВвод(цел pin) */
    // TODO implement me
    Q_UNUSED(pin)  // Remove this line on implementation;
    return false;
    
}

/* public slot */ void ArduinoModule::runDigitalWrite(const int pin, const bool value)
{
    /* алг цифрВывод(цел pin, лог value) */
    // TODO implement me
    Q_UNUSED(pin)  // Remove this line on implementation;
    Q_UNUSED(value)  // Remove this line on implementation;
    
}

/* public slot */ int ArduinoModule::runAnalogRead(const int pin)
{
    /* алг цел аналогВвод(цел pin) */
    // TODO implement me
    Q_UNUSED(pin)  // Remove this line on implementation;
    return 0;
    
}

/* public slot */ void ArduinoModule::runAnalogWrite(const int pin, const int value)
{
    /* алг аналогВывод(цел pin, цел value) */
    // TODO implement me
    Q_UNUSED(pin)  // Remove this line on implementation;
    Q_UNUSED(value)  // Remove this line on implementation;
    
}

/* public slot */ void ArduinoModule::runDelay(const int ms)
{
    /* алг задержкаС(цел ms) */
    // TODO implement me
    Q_UNUSED(ms)  // Remove this line on implementation;
    
}

/* public slot */ int ArduinoModule::runMilis()
{
    /* алг цел задержкаМС */
    // TODO implement me
    return 0;
    
}

/* public slot */ void ArduinoModule::runSerialbegin(const int rate)
{
    /* алг открытьПорт(цел rate) */
    // TODO implement me
    Q_UNUSED(rate)  // Remove this line on implementation;
    
}

/* public slot */ void ArduinoModule::runSerialprintln(const int data)
{
    /* алг выводВПорт(цел data) */
    // TODO implement me
    Q_UNUSED(data)  // Remove this line on implementation;
    
}

/* public slot */ int ArduinoModule::runINPUT()
{
    /* алг цел режимВвод */
    // TODO implement me
    return 0;
    
}

/* public slot */ int ArduinoModule::runOUTPUT()
{
    /* алг цел режимВывод */
    // TODO implement me
    return 0;
    
}

/* public slot */ int ArduinoModule::runHIGH()
{
    /* алг цел высокийСигнал */
    // TODO implement me
    return 0;
    
}

/* public slot */ int ArduinoModule::runLOW()
{
    /* алг цел низкийСигнал */
    // TODO implement me
    return 0;
    
}



} // namespace ActorArduino
