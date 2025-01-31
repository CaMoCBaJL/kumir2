#ifndef ARDUINOCOMPILERPLUGIN_H
#define ARDUINOCOMPILERPLUGIN_H

#include <kumir2-libs/extensionsystem/kplugin.h>
#include <kumir2/analizerinterface.h>
#include <kumir2/generatorinterface.h>
#include <kumir2-libs/extensionsystem/pluginspec.h>

namespace ArduinoCompilerTool {

class ArduinoCompilerToolPlugin
  : public ExtensionSystem::KPlugin
{
    Q_OBJECT
#if QT_VERSION >= 0x050000
    Q_PLUGIN_METADATA(IID "kumir2.ArduinoCompilerTool")
#endif
public:
    ArduinoCompilerToolPlugin();

    QString initialize(
            const QStringList & configurationArguments,
            const ExtensionSystem::CommandLine & runtimeArguments
            );
    QList<ExtensionSystem::CommandLineParameter> acceptableCommandLineParameters() const;
    void start();
    void stop();
    inline void updateSettings(const QStringList &) {}
protected:
    void createPluginSpec();
private:

    Shared::AnalizerInterface * analizer_;
    Shared::GeneratorInterface * generator_;
    QString sourceFileName_;
    QString sourceFileEncoding_;
    bool useAnsiWindowsOutput_;
    QString outFileName_;

};

}

#endif // ARDUINOCOMPILERPLUGIN_H
