/*
This file is generated, but you can safely change it
until you run "gen_actor_source.py" with "--project" flag.

Generated file is just a skeleton for module contents.
You should change it corresponding to functionality.
*/

#ifndef ARDUINOMODULE_H
#define ARDUINOMODULE_H

// Base class include
#include "arduinomodulebase.h"

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


class ArduinoModule
    : public ArduinoModuleBase
{
    Q_OBJECT
public /* methods */:
    ArduinoModule(ExtensionSystem::KPlugin * parent);
    static QList<ExtensionSystem::CommandLineParameter> acceptableCommandLineParameters();
    QWidget* mainWidget() const;
    QWidget* pultWidget() const;
public Q_SLOTS:
    void changeGlobalState(ExtensionSystem::GlobalState old, ExtensionSystem::GlobalState current);
    void loadActorData(QIODevice * source);
    void reloadSettings(ExtensionSystem::SettingsPtr settings, const QStringList & keys);
    void reset();
    void setAnimationEnabled(bool enabled);
    void terminateEvaluation();
    bool runDigitalRead(const int pin);
    void runDigitalWrite(const int pin, const bool value);
    int runAnalogRead(const int pin);
    void runAnalogWrite(const int pin, const int value);
    void runDelay(const int ms);
    int runMilis();
    void runSerialbegin(const int rate);
    void runSerialprintln(const int data);
    int runINPUT();
    int runOUTPUT();
    int runHIGH();
    int runLOW();



    /* ========= CLASS PRIVATE ========= */






};
        

} // namespace ActorArduino

#endif // ARDUINOMODULE_H
