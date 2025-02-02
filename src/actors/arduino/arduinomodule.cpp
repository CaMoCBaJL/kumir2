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

ArduinoModule::ArduinoModule(ExtensionSystem::KPlugin * parent)
    : ArduinoModuleBase(parent)
{
    // Module constructor, called once on plugin load
    // TODO implement me
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

/* public slot */ void ArduinoModule::runPinMode(const int pinMode)
{
    /* алг задатьПорт(цел pinMode) */
    // TODO implement me
    Q_UNUSED(pinMode)  // Remove this line on implementation;
    
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

/* public slot */ int ArduinoModule::runMin(const int x, const int y)
{
    /* алг цел мин(цел x, цел y) */
    // TODO implement me
    Q_UNUSED(x)  // Remove this line on implementation;
    Q_UNUSED(y)  // Remove this line on implementation;
    return 0;
    
}

/* public slot */ int ArduinoModule::runMax(const int x, const int y)
{
    /* алг цел макс(цел x, цел y) */
    // TODO implement me
    Q_UNUSED(x)  // Remove this line on implementation;
    Q_UNUSED(y)  // Remove this line on implementation;
    return 0;
    
}

/* public slot */ int ArduinoModule::runRandomSeed(const int seed)
{
    /* алг цел установитьСлуч(цел seed) */
    // TODO implement me
    Q_UNUSED(seed)  // Remove this line on implementation;
    return 0;
    
}

/* public slot */ int ArduinoModule::runRandom(const int max)
{
    /* алг цел случ(цел max) */
    // TODO implement me
    Q_UNUSED(max)  // Remove this line on implementation;
    return 0;
    
}

/* public slot */ int ArduinoModule::runRandom(const int min, const int max)
{
    /* алг цел случ(цел min, цел max) */
    // TODO implement me
    Q_UNUSED(min)  // Remove this line on implementation;
    Q_UNUSED(max)  // Remove this line on implementation;
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
    return 1;
}

/* public slot */ int ArduinoModule::runOUTPUT()
{
    return 0;
}

/* public slot */ int ArduinoModule::runHIGH()
{
    return 1;
}

/* public slot */ int ArduinoModule::runLOW()
{
    return 0; 
}



} // namespace ActorArduino
