#include "generator.h"
#include "arduino_tableelem.hpp"
#include <kumir2-libs/extensionsystem/pluginmanager.h>
#include <kumir2/actorinterface.h>
#include <kumir2-libs/dataformats/lexem.h>
#include <kumir2-libs/stdlib/kumirstdlib.hpp>

#include "arduino_data.hpp"
#include "entities/arduinoVariable.h"
#include <deque>
#include <cmath>
#include <limits>
#include "enums/enums.h"
#include "entities/arduinoInstruction.h"
#include "entities/arduinoTableElem.h"
#include "entities/arduinoData.h"
#include "generators/generators.h"
#include "converters/ASTToArduinoConverter.h"
#include "expressionCalculator.h"

namespace ArduinoCodeGenerator {

    using namespace Shared;
    using Arduino::ElemType;
    using Kumir::Core;

    template<typename T>
    std::vector <T> &operator<<(std::vector <T> &vec, const T &value) {
        vec.push_back(value);
        return vec;
    }


    template<typename T>
    std::deque <T> &operator<<(std::deque <T> &vec, const T &value) {
        vec.push_back(value);
        return vec;
    }

    Generator::Generator(QObject *parent)
            : QObject(parent), byteCode_(nullptr), debugLevel_(Shared::GeneratorInterface::LinesAndVariables) {
                this->loopIteratorCounter_ = 0;
    }

    void Generator::setDebugLevel(DebugLevel debugLevel) {
        debugLevel_ = debugLevel;
    }

    void Generator::reset(const AST::DataPtr ast, Arduino::Data *bc) {
        ast_ = ast;
        byteCode_ = bc;
        constants_.clear();
        externs_.clear();
    }

    static void getVarListSizes(const QVariant &var, int sizes[3], int fromDim) {
        if (fromDim > 2)
            return;
        sizes[0] = sizes[1] = sizes[2] = 1;
        QVariantList elems = var.toList();
        for (int i = 0; i < elems.size(); i++) {
            if (elems.at(i).type() == QVariant::List) {
                getVarListSizes(elems[i], sizes, fromDim + 1);
            }
        }
        sizes[fromDim] = qMax(sizes[fromDim], elems.size());
    }

    QList<QVariant> Generator::getConstantValues(){
        QList<QVariant> result = QList<QVariant>();
        for (uint16_t i = 0; i < constants_.size();i++){
            result.append(constants_[i].value);
        }

        return result;
    }

    static Arduino::CAnyValue
    makeAnyValue(const QVariant &val, const QList <Arduino::ValueType> &vt, const QString &moduleName,
                 const QString &className
    ) {
        Arduino::CAnyValue result;
        if (val == QVariant::Invalid)
            return result;
        switch (vt.front()) {
            case Arduino::VT_int:
                result = Arduino::CAnyValue(val.toInt());
                break;
            case Arduino::VT_float:
                result = Arduino::CAnyValue(val.toDouble());
                break;
            case Arduino::VT_bool:
                result = Arduino::CAnyValue(bool(val.toBool()));
                break;
            case Arduino::VT_char:
                result = Arduino::CAnyValue(Kumir::Char(val.toChar().unicode()));
                break;
            case Arduino::VT_string:
                result = Arduino::CAnyValue(val.toString().toStdWString());
                break;
            case Arduino::VT_struct: {
                QVariantList valueFields = val.toList();
                Arduino::Record record;
                for (int i = 1; i < vt.size(); i++) {
                    Arduino::ValueType fieldType = vt.at(i);
                    switch (fieldType) {
                        case Arduino::VT_int:
                            record.fields.push_back(Arduino::CAnyValue(valueFields[i - 1].toInt()));
                            break;
                        case Arduino::VT_struct:
                            record.fields.push_back(Arduino::CAnyValue(valueFields[i - 1].toDouble()));
                            break;
                        case Arduino::VT_bool:
                            record.fields.push_back(Arduino::CAnyValue(valueFields[i - 1].toBool()));
                            break;
                        case Arduino::VT_char:
                            record.fields.push_back(Arduino::CAnyValue(valueFields[i - 1].toChar().unicode()));
                            break;
                        case Arduino::VT_string:
                            record.fields.push_back(Arduino::CAnyValue(valueFields[i - 1].toString().toStdWString()));
                            break;
                        default:
                            break;
                    }
                }
                result = Arduino::CAnyValue(record);
                break;
            }
            default:
                break;
        }
        return result;
    }

    quint16 Generator::calculateConstantValue(const QList <Arduino::ValueType> &type, quint8 dimension, const QVariant &value,
                                     const QString &moduleName, const QString &className
    ) {
        ConstValue c;
        c.baseType = type;
        c.dimension = dimension;
        c.value = value;
        c.recordModuleName = moduleName;
        c.recordClassLocalizedName = className;
        if (!constants_.contains(c)) {
            constants_ << c;
            return constants_.size() - 1;
        } else {
            return constants_.indexOf(c);
        }
    }

    static QString createMethodSignature(const AST::AlgorithmPtr alg) {
        const QString rt = typeToSignature(alg->header.returnType);
        QStringList args;
        for (int i = 0; i < alg->header.arguments.size(); i++) {
            QString sarg;
            const AST::VariablePtr karg = alg->header.arguments[i];
            if (karg->accessType == AST::AccessArgumentInOut)
                sarg = "inout ";
            else if (karg->accessType == AST::AccessArgumentOut)
                sarg = "out ";
            else
                sarg = "in ";
            sarg += typeToSignature(karg->baseType);
            for (uint8_t j = 0; j < karg->dimension; j++) {
                sarg += "[]";
            }
            args << sarg;
        }
        const QString argline = args.size() > 0
                                ? args.join(",") : QString();
        const QString result = argline.length() > 0
                               ? rt + ":" + argline : rt;
        return result;
    }

    void Generator::generateExternTable() {
        QSet <AST::ModulePtr> modulesImplicitlyImported;
        for (int i = externs_.size() - 1; i >= 0; i--) {
            QPair <quint8, quint16> ext = externs_[i];
            Arduino::TableElem e;
            e.type = Arduino::EL_EXTERN;
            e.module = ext.first;
            const AST::ModulePtr mod = ast_->modules[ext.first];
            const QList <AST::AlgorithmPtr> table = mod->header.algorhitms + mod->header.operators;
            const AST::AlgorithmPtr alg = table[ext.second];
            if (alg->header.external.moduleName.endsWith(".kum") ||
                alg->header.external.moduleName.endsWith(".kod")) {
                e.algId = e.id = ext.second;
            } else {
                e.algId = e.id = alg->header.external.id;
            }
            QList < ExtensionSystem::KPlugin * > plugins =
                    ExtensionSystem::PluginManager::instance()->loadedPlugins("Actor*");
            QString moduleFileName;
            QString signature;
            for (int m = 0; m < plugins.size(); m++) {
                Shared::ActorInterface *actor = qobject_cast<Shared::ActorInterface *>(plugins[m]);
                if (actor && actor->asciiModuleName() == mod->header.asciiName) {
                    signature = createMethodSignature(alg);
                    moduleFileName = plugins[m]->pluginSpec().libraryFileName;
                    // Make filename relative
                    int slashP = moduleFileName.lastIndexOf("/");
                    if (slashP != -1) {
                        moduleFileName = moduleFileName.mid(slashP + 1);
                    }
                }
            }
            if (mod->header.type == AST::ModTypeExternal)
                modulesImplicitlyImported.insert(mod);
            if (mod->header.type == AST::ModTypeCached)
                moduleFileName = mod->header.name;
            e.moduleAsciiName = std::string(mod->header.asciiName.constData());
            e.moduleLocalizedName = mod->header.name.toStdWString();
            e.name = alg->header.name.toStdWString();
            e.signature = signature.toStdWString();
            e.fileName = moduleFileName.toStdWString();
            byteCode_->d.push_front(e);
        }
        QSet <AST::ModulePtr> modulesExplicitlyImported;
        for (int i = 0; i < ast_->modules.size(); i++) {
            AST::ModulePtr module = ast_->modules[i];
            if (module->header.type == AST::ModTypeExternal &&
                module->builtInID == 0x00
                    ) {
                const QList <AST::ModuleWPtr> &used = module->header.usedBy;
                for (int j = 0; j < used.size(); j++) {
                    AST::ModuleWPtr reference = used[j];
                    const AST::ModuleHeader &header = reference.data()->header;
                    if (header.type == AST::ModTypeUser ||
                        header.type == AST::ModTypeUserMain ||
                        header.type == AST::ModTypeTeacher ||
                        header.type == AST::ModTypeTeacherMain
                            ) {
                        modulesExplicitlyImported.insert(module);
                        break;
                    }
                }
            }
        }
        foreach(AST::ModulePtr
        module, modulesExplicitlyImported - modulesImplicitlyImported) {
            Arduino::TableElem e;
            e.type = Arduino::EL_EXTERN_INIT;
            e.module = 0xFF;
            e.algId = e.id = 0xFFFF;
            e.moduleLocalizedName = module->header.name.toStdWString();
            QString moduleFileName;
            QList < ExtensionSystem::KPlugin * > plugins =
                    ExtensionSystem::PluginManager::instance()->loadedPlugins("Actor*");
            for (int m = 0; m < plugins.size(); m++) {
                Shared::ActorInterface *actor = qobject_cast<Shared::ActorInterface *>(plugins[m]);
                if (actor && actor->localizedModuleName(QLocale::Russian) == module->header.name) {
                    moduleFileName = plugins[m]->pluginSpec().libraryFileName;
                    // Make filename relative
                    int slashP = moduleFileName.lastIndexOf("/");
                    if (slashP != -1) {
                        moduleFileName = moduleFileName.mid(slashP + 1);
                    }
                }
            }
            e.moduleAsciiName = std::string(module->header.asciiName.constData());
            e.moduleLocalizedName = module->header.name.toStdWString();
            e.fileName = moduleFileName.toStdWString();
            byteCode_->d.push_front(e);
        }
    }

    void Generator::addModule(const AST::ModulePtr mod) {
        int id = ast_->modules.indexOf(mod);
        if (mod->header.type == AST::ModTypeExternal) {

        } else {
            addKumirModule(id, mod);
        }
    }


    void Generator::addKumirModule(int id, const AST::ModulePtr mod) {
        for (int i = 0; i < mod->impl.globals.size(); i++) {
            const AST::VariablePtr var = mod->impl.globals[i];
            Arduino::TableElem glob;
            glob.type = Arduino::EL_GLOBAL;
            glob.module = quint8(id);
            glob.id = quint16(i);
            glob.name = var->name.toStdWString();
            glob.dimension = quint8(var->dimension);
            glob.vtype = parseValueType(var->baseType).toStdList();
            glob.refvalue = parseValueKind(var->accessType);
            glob.recordModuleAsciiName = var->baseType.actor ?
                                         std::string(var->baseType.actor->asciiModuleName().constData())
                                                             : std::string();
            glob.recordModuleLocalizedName = var->baseType.actor ?
                                             var->baseType.actor->localizedModuleName(QLocale::Russian).toStdWString()
                                                                 : std::wstring();
            glob.recordClassLocalizedName = var->baseType.name.toStdWString();
            byteCode_->d.push_back(glob);
        }
        Arduino::TableElem initElem;
        Arduino::Instruction returnFromInit;
        returnFromInit.type = Arduino::RET;
        initElem.type = Arduino::EL_INIT;
        initElem.module = quint8(id);
        initElem.moduleLocalizedName = mod->header.name.toStdWString();
        initElem.instructions = calculateInstructions(id, -1, 0, mod->impl.initializerBody).toVector().toStdVector();
        if (!initElem.instructions.empty())
            initElem.instructions << returnFromInit;
        if (!initElem.instructions.empty())
            byteCode_->d.push_back(initElem);
        AST::ModulePtr mainMod;
        AST::AlgorithmPtr mainAlg;
        int mainModId = -1;
        int mainAlgorhitmId = -1;
        for (int i = 0; i < mod->impl.algorhitms.size(); i++) {
            const AST::AlgorithmPtr alg = mod->impl.algorhitms[i];
            Arduino::ElemType ft = Arduino::EL_FUNCTION;
            if (mod->header.name.isEmpty() && i == 0) {
                ft = Arduino::EL_MAIN;
                if (!alg->header.arguments.isEmpty() || alg->header.returnType.kind != AST::TypeNone) {
                    mainMod = mod;
                    mainAlg = alg;
                    mainModId = id;
                    mainAlgorhitmId = i;
                }
            }
            if (alg->header.specialType == AST::AlgorithmTypeTesting) {
                ft = Arduino::EL_TESTING;
            }
            addFunction(i, id, ft, mod, alg);
        }
        if (mainMod && mainAlg) {
            addInputArgumentsMainAlgorhitm(mainModId, mainAlgorhitmId, mainMod, mainAlg);
        }
    }

    QList <Arduino::Instruction> Generator::calculateInstructions(
            int modId, int algId, int level,
            const QList <AST::StatementPtr> &statements) {

        QList <Arduino::Instruction> result;
        for (int i = 0; i < statements.size(); i++) {
            const AST::StatementPtr st = statements[i];
            switch (st->type) {
                case AST::StError:
                    if (!st->skipErrorEvaluation)
                        generateErrorInstruction(modId, algId, level, st, result);
                    break;
                case AST::StAssign:
                    generateAssignInstruction(modId, algId, level, st, result);
                    break;
                case AST::StAssert:
                    generateAssertInstruction(modId, algId, level, st, result);
                    break;
                case AST::StVarInitialize:
                    generateVarInitInstruction(modId, algId, level, st, result);
                    break;
                case AST::StInput:
                    generateTerminalIOInstruction(modId, algId, level, st, result);
                    break;
                case AST::StOutput:
                    generateTerminalIOInstruction(modId, algId, level, st, result);
                    break;
                case AST::StLoop:
                    generateLoopInstruction(modId, algId, level + 1, st, result);
                    break;
                case AST::StIfThenElse:
                    generateConditionInstruction(modId, algId, level, st, result);
                    break;
                case AST::StSwitchCaseElse:
                    generateChoiceInstruction(modId, algId, level, st, result);
                    break;
                case AST::StHalt:
                case AST::StBreak:
                case AST::StPause:
                    generateBreakInstruction(modId, algId, level, st, result);
                    break;
                default:
                    break;
            }

            Arduino::Instruction delimiter;
            delimiter.type = Arduino::STR_DELIM;
            result << delimiter;
        }
        return result;
    }

    quint16 Generator::calculateConstantValue(Arduino::ValueType type, quint8 dimension, const QVariant &value,
                                     const QString &moduleName, const QString &className) {
        QList <Arduino::ValueType> vt;
        vt.push_back(type);
        return calculateConstantValue(vt, dimension, value, QString(), QString());
    }

    void Generator::findVariable(int modId, int algId, const AST::VariablePtr var, Arduino::VariableScope &scope,
                                 quint16 &id) const {
        const AST::ModulePtr mod = ast_->modules.at(modId);
        for (quint16 i = 0; i < mod->impl.globals.size(); i++) {
            if (mod->impl.globals.at(i) == var) {
                scope = Arduino::GLOBAL;
                id = i;
                return;
            }
        }
        const AST::AlgorithmPtr alg = mod->impl.algorhitms[algId];
        for (quint16 i = 0; i < alg->impl.locals.size(); i++) {
            if (alg->impl.locals.at(i) == var) {

                scope = Arduino::LOCAL;
                id = i;
                return;
            }
        }
    }

    void Generator::findFunction(const AST::AlgorithmPtr alg, quint8 &module, quint16 &id) {
        for (quint8 i = 0; i < ast_->modules.size(); i++) {
            const AST::ModulePtr mod = ast_->modules[i];
            QList <AST::AlgorithmPtr> table;
            if (mod->header.type == AST::ModTypeExternal || mod->header.type == AST::ModTypeCached)
                table = mod->header.algorhitms + mod->header.operators;
            else
                table = mod->impl.algorhitms + mod->header.operators;
            for (quint16 j = 0; j < table.size(); j++) {
                if (alg == table[j]) {
                    module = i;
                    id = j;
                    if (mod->header.type == AST::ModTypeExternal) {
                        id = alg->header.external.id;
                    }
                    if (mod->header.type == AST::ModTypeCached ||
                        (mod->header.type == AST::ModTypeExternal && (mod->builtInID & 0xF0) == 0)) {
                        QPair <quint8, quint16> ext(module, id);
                        if (!externs_.contains(ext))
                            externs_ << ext;
                    }
                    if (mod->builtInID)
                        module = mod->builtInID;
                    return;
                }
            }
        }
    }
} // namespace ArduinoCodeGenerator
