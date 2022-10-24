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
#include <QTime>
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
public Q_SLOTS:
    void changeGlobalState(ExtensionSystem::GlobalState old, ExtensionSystem::GlobalState current);
    void loadActorData(QIODevice * source);
    void reloadSettings(ExtensionSystem::SettingsPtr settings, const QStringList & keys);
    void reset();
    void terminateEvaluation();
    bool runDigitalRead(const int pin);
    void runDigitalWrite(const int pin, const bool value);
    int runAnalogRead(const int pin);
    void runAnalogWrite(const int pin, const int value);
    void runPinMode(const int pinMode);
    void runDelay(const int ms);
    int runMilis();
    int runMin(const int x, const int y);
    int runMax(const int x, const int y);
    int runRandomSeed(const int seed);
    int runRandom(const int max);
    int runRandom(const int min, const int max);
    void runSerialbegin(const int rate);
    void runSerialprintln(const int data);
    int runINPUT();
    int runOUTPUT();
    int runHIGH();
    int runLOW();



    /* ========= CLASS PRIVATE ========= */
private:
    QTime *milisTimer;





};
        

} // namespace ActorArduino

#endif // ARDUINOMODULE_H
