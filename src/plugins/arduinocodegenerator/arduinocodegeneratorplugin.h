#ifndef ARDUINOCODEGENERATORPLUGIN_H
#define ARDUINOCODEGENERATORPLUGIN_H

#include <kumir2-libs/extensionsystem/kplugin.h>
#include <kumir2/generatorinterface.h>
#include <kumir2-libs/dataformats/ast_module.h>

#include <QObject>
#include <QtPlugin>

using namespace Shared;

namespace ArduinoCodeGenerator {

class ArduinoCodeGeneratorPlugin
        : public ExtensionSystem::KPlugin
        , public GeneratorInterface
{
    Q_OBJECT
#if QT_VERSION >= 0x050000
    Q_PLUGIN_METADATA(IID "kumir2.ArduinoCodeGenerator")
#endif
    Q_INTERFACES(Shared::GeneratorInterface)

public:
    ArduinoCodeGeneratorPlugin();

    QList<ExtensionSystem::CommandLineParameter> acceptableCommandLineParameters() const;

    void setDebugLevel(DebugLevel debugLevel);
    void generateExecutable(
                const AST::DataPtr tree,
                QByteArray & out,
                QString & mimeType,
                QString & fileSuffix
                );

    void setOutputToText(bool flag);
    void setSerialDebug(bool flag);
    inline void setVerbose(bool) {}
    inline void setTemporaryDir(const QString &, bool ) {}
    inline void updateSettings(const QStringList &) {}
protected:
    void createPluginSpec();
    QString initialize(const QStringList &configurationArguments,
                       const ExtensionSystem::CommandLine &runtimeArguments);
    void start();
    void stop();
private:
    class Generator * d;
    bool textMode_;
    bool serialDebug_;
};

}

#endif // ARDUINOCODEGENERATORPLUGIN_H
