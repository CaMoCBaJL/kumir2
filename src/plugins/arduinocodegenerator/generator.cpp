#include "generator.h"
#include "arduino_tableelem.hpp"
#include <kumir2-libs/extensionsystem/pluginmanager.h>
#include <kumir2/actorinterface.h>
#include <kumir2-libs/dataformats/lexem.h>
#include <kumir2-libs/stdlib/kumirstdlib.hpp>

#include "arduino_data.hpp"
#include "arduino_enums.hpp"
#include "CVariable.hpp"
#include <deque>
#include <cmath>
#include <limits>

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

    Arduino::ValueType parseVarType(AST::VariablePtr variable){
        switch (variable->baseType.kind) {
            case AST::TypeInteger:
                return Arduino::VT_int;
            case AST::TypeBoolean:
                return Arduino::VT_bool;
            case AST::TypeCharect:
                return Arduino::VT_char;
            case AST::TypeReal:
                return Arduino::VT_float;
            case AST::TypeString:
                return Arduino::VT_string;
            case AST::TypeUser:
                return Arduino::VT_struct;
            case AST::TypeNone:
                return Arduino::VT_void;
            default:
                return Arduino::VT_void;
        }
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

    quint16 Generator::constantValue(const QList <Arduino::ValueType> &type, quint8 dimension, const QVariant &value,
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

    static QString typeToSignature(const AST::Type &t) {
        QString result;
        if (t.kind == AST::TypeNone)
            result = "void";
        else if (t.kind == AST::TypeBoolean)
            result = "bool";
        else if (t.kind == AST::TypeInteger)
            result = "int";
        else if (t.kind == AST::TypeReal)
            result = "float";
        else if (t.kind == AST::TypeCharect)
            result = "char";
        else if (t.kind == AST::TypeString)
            result = "String";
        else if (t.kind == AST::TypeUser) {
            result = "struct {";
            for (int i = 0; i < t.userTypeFields.size(); i++) {
                if (i > 0)
                    result += ";";
                result += typeToSignature(t.userTypeFields.at(i).second);
            }
            result += "}";
        }
        Q_ASSERT(result.length() > 0);
        return result;
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


    QList <Arduino::ValueType> Generator::valueType(const AST::Type &t) {
        QList <Arduino::ValueType> result;
        if (t.kind == AST::TypeInteger)
            result << Arduino::VT_int;
        else if (t.kind == AST::TypeReal)
            result << Arduino::VT_float;
        else if (t.kind == AST::TypeBoolean)
            result << Arduino::VT_bool;
        else if (t.kind == AST::TypeString)
            result << Arduino::VT_string;
        else if (t.kind == AST::TypeCharect)
            result << Arduino::VT_char;
        else if (t.kind == AST::TypeUser) {
            result << Arduino::VT_struct;
            for (int i = 0; i < t.userTypeFields.size(); i++) {
                AST::Field field = t.userTypeFields[i];
                result << valueType(field.second);
            }
        } else
            result << Arduino::VT_void;
        return result;
    }

    Arduino::ValueKind Generator::valueKind(AST::VariableAccessType t) {
        if (t == AST::AccessArgumentIn)
            return Arduino::VK_In;
        else if (t == AST::AccessArgumentOut)
            return Arduino::VK_Out;
        else if (t == AST::AccessArgumentInOut)
            return Arduino::VK_InOut;
        else
            return Arduino::VK_Plain;
    }

    Arduino::InstructionType Generator::operation(AST::ExpressionOperator op) {
        if (op == AST::OpSumm)
            return Arduino::SUM;
        else if (op == AST::OpSubstract)
            return Arduino::SUB;
        else if (op == AST::OpMultiply)
            return Arduino::MUL;
        else if (op == AST::OpDivision)
            return Arduino::DIV;
        else if (op == AST::OpPower)
            return Arduino::POW;
        else if (op == AST::OpNot)
            return Arduino::NEG;
        else if (op == AST::OpAnd)
            return Arduino::AND;
        else if (op == AST::OpOr)
            return Arduino::OR;
        else if (op == AST::OpEqual)
            return Arduino::EQ;
        else if (op == AST::OpNotEqual)
            return Arduino::NEQ;
        else if (op == AST::OpLess)
            return Arduino::LS;
        else if (op == AST::OpGreater)
            return Arduino::GT;
        else if (op == AST::OpLessOrEqual)
            return Arduino::LEQ;
        else if (op == AST::OpGreaterOrEqual)
            return Arduino::GEQ;
        else
            return Arduino::NOP;
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
            glob.vtype = valueType(var->baseType).toStdList();
            glob.refvalue = valueKind(var->accessType);
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
        initElem.instructions = instructions(id, -1, 0, mod->impl.initializerBody).toVector().toStdVector();
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

    void Generator::addInputArgumentsMainAlgorhitm(int moduleId, int algorhitmId, const AST::ModulePtr mod,
                                                   const AST::AlgorithmPtr alg) {
        // Generate hidden algorhitm, which will called before main to input arguments
        int algId = mod->impl.algorhitms.size();
        QList <Arduino::Instruction> instrs;
        QList <quint16> varsToOut;
        int locOffset = 0;

        // Add function return as local
        if (alg->header.returnType.kind != AST::TypeNone) {
            const AST::VariablePtr retval = returnValue(alg);
            Arduino::TableElem loc;
            loc.type = Arduino::EL_LOCAL;
            loc.module = moduleId;
            loc.algId = algId;
            loc.id = 0;
            loc.name = tr("Function return value").toStdWString();
            loc.dimension = 0;
            loc.vtype = valueType(retval->baseType).toStdList();
            loc.refvalue = Arduino::VK_Plain;
            byteCode_->d.push_back(loc);
            varsToOut << constantValue(Arduino::VT_int, 0, 0, QString(), QString());
            locOffset = 1;
        }

        // Add arguments as locals
        for (int i = 0; i < alg->header.arguments.size(); i++) {
            const AST::VariablePtr var = alg->header.arguments[i];
            Arduino::TableElem loc;
            loc.type = Arduino::EL_LOCAL;
            loc.module = moduleId;
            loc.algId = algId;
            loc.id = locOffset + i;
            loc.name = var->name.toStdWString();
            loc.dimension = var->dimension;
            loc.vtype = valueType(var->baseType).toStdList();
            loc.recordModuleAsciiName = var->baseType.actor ?
                                        std::string(var->baseType.actor->asciiModuleName().constData()) : std::string();
            loc.recordModuleLocalizedName = var->baseType.actor ?
                                            var->baseType.actor->localizedModuleName(QLocale::Russian).toStdWString()
                                                                : std::wstring();
            loc.recordClassLocalizedName = var->baseType.name.toStdWString();
            loc.recordClassAsciiName = std::string(var->baseType.asciiName.constData());
            loc.refvalue = Arduino::VK_Plain;
            byteCode_->d.push_back(loc);

        }

        for (int i = 0; i < alg->header.arguments.size(); i++) {
            AST::VariablePtr var = alg->header.arguments[i];
            // Initialize argument
            if (var->dimension > 0) {
                for (int j = var->dimension - 1; j >= 0; j--) {
                    QList <Arduino::Instruction> initBounds;
                    initBounds << calculate(moduleId, algorhitmId, 0, var->bounds[j].second);
                    initBounds << calculate(moduleId, algorhitmId, 0, var->bounds[j].first);
                    for (int i_bounds = 0; i_bounds < initBounds.size(); i_bounds++) {
                        Arduino::Instruction &instr = initBounds[i_bounds];
                        if (instr.type == Arduino::LOAD || instr.type == Arduino::LOADARR) {
//                        if (instr.scope==Arduino::LOCAL)
//                            instr.arg = uint16_t(instr.arg-locOffset);
                        }
                    }
                    instrs << initBounds;
                }
                Arduino::Instruction bounds;
                bounds.type = Arduino::SETARR;
                bounds.scope = Arduino::LOCAL;
                bounds.arg = quint16(i + locOffset);
                instrs << bounds;
            }
            // If IN of INOUT argument -- require input
            // This made by special function call
            if (var->accessType == AST::AccessArgumentIn || var->accessType == AST::AccessArgumentInOut) {
                Arduino::Instruction varId;
                varId.type = Arduino::LOAD;
                varId.scope = Arduino::CONSTT;
                varId.arg = constantValue(Arduino::VT_int, 0, i + locOffset, QString(), QString());

                Arduino::Instruction call;
                call.type = Arduino::CALL;
                instrs << varId << call;
            }
            if (var->accessType == AST::AccessArgumentOut || var->accessType == AST::AccessArgumentInOut) {
                varsToOut << constantValue(Arduino::VT_int, 0, i + locOffset, QString(), QString());
            }
        }

        // Call main (first) algorhitm:
        //  -- 1) Push arguments
        for (int i = 0; i < alg->header.arguments.size(); i++) {
            AST::VariableAccessType t = alg->header.arguments[i]->accessType;
            if (t == AST::AccessArgumentIn) {
                Arduino::Instruction load;
                load.type = Arduino::LOAD;
                load.scope = Arduino::LOCAL;
                load.arg = quint16(i + locOffset);
                instrs << load;
            } else if (t == AST::AccessArgumentOut || t == AST::AccessArgumentInOut) {
                Arduino::Instruction ref;
                ref.type = Arduino::REF;
                ref.scope = Arduino::LOCAL;
                ref.arg = quint16(i + locOffset);
                instrs << ref;
            }
        }

        Arduino::Instruction argsCount;
        argsCount.type = Arduino::LOAD;
        argsCount.scope = Arduino::CONSTT;
        argsCount.arg = constantValue(QList<Arduino::ValueType>() << Arduino::VT_int, 0, alg->header.arguments.size(),
                                      QString(), QString());
        instrs << argsCount;

        //  -- 2) Call main (first) algorhitm
        Arduino::Instruction callInstr;
        callInstr.type = Arduino::CALL;
        findFunction(alg, callInstr.module, callInstr.arg);
        instrs << callInstr;
        //  -- 3) Store return value
        if (alg->header.returnType.kind != AST::TypeNone) {

        }

        // Show what in result...

        for (int i = 0; i < varsToOut.size(); i++) {
            Arduino::Instruction arg;
            arg.type = Arduino::LOAD;
            arg.scope = Arduino::CONSTT;
            arg.arg = varsToOut[i];
            instrs << arg;
            Arduino::Instruction callShow;
            callShow.type = Arduino::CALL;
            instrs << callShow;
        }

        Arduino::TableElem func;
        func.type = Arduino::EL_BELOWMAIN;
        func.algId = func.id = algId;
        func.module = moduleId;
        func.moduleLocalizedName = mod->header.name.toStdWString();
        func.name = QString::fromLatin1("@below_main").toStdWString();
        func.instructions = instrs.toVector().toStdVector();
        byteCode_->d.push_back(func);

    }

    std::string GetOperatorType(Arduino::Instruction instr){
        switch (instr.type){
            case Arduino::EQ: return "==";
            case Arduino::NEQ: return "!=";
            case Arduino::OR: return "||";
            case Arduino::LS: return "<";
            case Arduino::LEQ: return "<=";
            case Arduino::GT: return ">";
            case Arduino::GEQ: return ">=";
            case Arduino::AND: return "&&";
            case Arduino::VAR: return "variable";
            case Arduino::CONST: return "constant";
            default: return std::to_string(instr.type);
        }
    }

    QString typeSignature(const AST::Type &tp) {
        QString signature;
        if (tp.kind == AST::TypeNone) {
            signature += "void";
        } else if (tp.kind == AST::TypeInteger) {
            signature += "int";
        } else if (tp.kind == AST::TypeReal) {
            signature += "float";
        } else if (tp.kind == AST::TypeBoolean) {
            signature += "bool";
        } else if (tp.kind == AST::TypeCharect) {
            signature += "char";
        } else if (tp.kind == AST::TypeString) {
            signature += "string";
        } else if (tp.kind == AST::TypeUser) {
            signature += "struct " + tp.name + " {";
            for (int i = 0; i < tp.userTypeFields.size(); i++) {
                signature += typeSignature(tp.userTypeFields.at(i).second);
                if (i < tp.userTypeFields.size() - 1)
                    signature += ";";
            }
            signature += "}";
        }
        return signature;
    }

    int findLastInstruction(QList<Arduino::Instruction> instructions, Arduino::InstructionType type){
        for (int i = instructions.size() - 1; i > -1; --i){
            if (instructions.at(i).type == type) {
                return i;
            }
        }

        return -1;
    }

    void Generator::addFunction(int id, int moduleId, Arduino::ElemType type, const AST::ModulePtr mod,
                                const AST::AlgorithmPtr alg) {
        QString headerError = "";
        QString beginError = "";

        if (alg->impl.headerLexems.size() > 0) {
            for (int i = 0; i < alg->impl.headerLexems.size(); i++) {
                if (alg->impl.headerLexems[i]->error.size() > 0) {
                    headerError = ErrorMessages::message("KumirAnalizer", QLocale::Russian,
                                                         alg->impl.headerLexems[i]->error);
                    break;
                }
            }
        }

        if (alg->impl.beginLexems.size() > 0) {
            for (int i = 0; i < alg->impl.beginLexems.size(); i++) {
                if (alg->impl.beginLexems[i]->error.size() > 0) {
                    beginError = ErrorMessages::message("KumirAnalizer", QLocale::Russian,
                                                        alg->impl.beginLexems[i]->error);
                    break;
                }
            }
        }

        QString signature;

        signature = typeSignature(alg->header.returnType) + ":";

        for (int i = 0; i < alg->header.arguments.size(); i++) {
            const AST::VariablePtr var = alg->header.arguments[i];
            if (var->accessType == AST::AccessArgumentIn)
                signature += "in ";
            else if (var->accessType == AST::AccessArgumentOut)
                signature += "out ";
            else if (var->accessType == AST::AccessArgumentInOut)
                signature += "inout ";
            signature += typeSignature(var->baseType);
            for (int j = 0; j < var->dimension; j++) {
                signature += "[]";
            }
            if (i < alg->header.arguments.size() - 1)
                signature += ",";
        }

        Arduino::TableElem func;
        func.type = type;
        func.module = moduleId;
        func.algId = func.id = id;
        std::string algName = alg->header.name.toStdString();
        func.name = std::wstring(algName.begin(), algName.end());
        func.signature = signature.toStdWString();
        func.moduleLocalizedName = mod->header.name.toStdWString();
        QList <Arduino::Instruction> argHandle;


        if (headerError.length() > 0) {
            Arduino::Instruction err;
            err.type = Arduino::ERRORR;
            err.scope = Arduino::CONSTT;
            err.arg = constantValue(Arduino::VT_string, 0, headerError, QString(), QString());
            argHandle << err;
        }

        if (alg->impl.headerRuntimeError.size() > 0) {
            Arduino::Instruction l;
            argHandle << l;
            l.type = Arduino::ERRORR;
            l.scope = Arduino::CONSTT;
            l.arg = constantValue(Arduino::VT_string, 0,
                                  ErrorMessages::message("KumirAnalizer", QLocale::Russian,
                                                         alg->impl.headerRuntimeError), QString(), QString()
            );
            argHandle << l;
        }

        for (int i = 0; i < alg->header.arguments.size(); i++) {
            const AST::VariablePtr var = alg->header.arguments[i];
            if (var->dimension > 0) {
                for (int i = var->dimension - 1; i >= 0; i--) {
                    argHandle << calculate(moduleId, id, 0, var->bounds[i].second);
                    argHandle << calculate(moduleId, id, 0, var->bounds[i].first);
                }
                Arduino::Instruction bounds;
                bounds.type = Arduino::UPDARR;
                findVariable(moduleId, id, var, bounds.scope, bounds.arg);
                argHandle << bounds;
            } else {
                Arduino::Instruction headerArg;
                headerArg.type = Arduino::VAR;
                headerArg.varName = var->name;
                headerArg.varType = parseVarType(var);
                switch (var->accessType) {
                    case AST::AccessArgumentOut:
                        headerArg.kind = Arduino::VK_Out;
                        break;
                    case AST::AccessArgumentInOut:
                        headerArg.kind = Arduino::VK_InOut;
                        break;
                    case AST::AccessArgumentIn:
                        headerArg.kind = Arduino::VK_In;
                        break;
                    default:
                        headerArg.kind = Arduino::VK_Plain;
                        break;
                }

                argHandle << headerArg;
                if (i + 1 < alg->header.arguments.size()) {
                    headerArg.type = Arduino::END_ARG;
                    argHandle << headerArg;
                }
            }
        }

        Arduino::Instruction endFuncDeclaration;
        endFuncDeclaration.type = Arduino::END_ST_HEAD;
        argHandle << endFuncDeclaration;

        if (beginError.length() > 0) {
            Arduino::Instruction err;
            err.type = Arduino::ERRORR;
            err.scope = Arduino::CONSTT;
            err.arg = constantValue(Arduino::VT_string, 0, beginError, QString(), QString());
            argHandle << err;
        }

        QList <Arduino::Instruction> pre = instructions(moduleId, id, 0, alg->impl.pre);
        QList <Arduino::Instruction> body = instructions(moduleId, id, 0, alg->impl.body);
        QList <Arduino::Instruction> post = instructions(moduleId, id, 0, alg->impl.post);

        int offset = argHandle.size() + pre.size();
        offset += body.size();
        QList <Arduino::Instruction> ret;

        int retIp = argHandle.size() + pre.size() + body.size() + post.size();

        if (alg->impl.endLexems.size() > 0) {
            QString endError;
            for (int i = 0; i < alg->impl.endLexems.size(); i++) {
                if (alg->impl.endLexems[i]->error.size() > 0) {
                    endError = ErrorMessages::message("KumirAnalizer", QLocale::Russian, alg->impl.endLexems[i]->error);
                    break;
                }
            }
            if (endError.length() > 0) {
                Arduino::Instruction err;
                err.type = Arduino::ERRORR;
                err.scope = Arduino::CONSTT;
                err.arg = constantValue(Arduino::VT_string, 0, endError, QString(), QString());
                ret << err;
            }
        }

        Arduino::Instruction retturn;
        retturn.type = Arduino::RET;
        ret << retturn;

        const AST::VariablePtr retval = returnValue(alg);
        if (retval != nullptr) {
            qCritical() << "function " << alg->header.name << "returns " << retval->name;
            Arduino::Instruction loadRetval;
            loadRetval.type = Arduino::VAR;
            loadRetval.varName = retval->name;
            ret << loadRetval;
        } else {
//            qCritical() << "\nbefore:\n";
//            for (int i = 0; i < argHandle.size(); ++i){
//                qCritical() << "instr: " << std::to_string(argHandle.at(i).type).c_str();
//            }
//            qCritical() << "\n";

            int lastArgIndex = findLastInstruction(argHandle, Arduino::VAR);
            if (lastArgIndex > -1 && argHandle.at(lastArgIndex).kind > 1) {
                Arduino::Instruction funcResult;
                funcResult.type = Arduino::VAR;
                funcResult.varName = argHandle.at(lastArgIndex).varName;
                ret << funcResult;
                Arduino::Instruction newRetArg = argHandle.at(lastArgIndex);
                newRetArg.varName = QString::null;

                argHandle.removeAt(lastArgIndex);
                argHandle.insert(lastArgIndex, newRetArg);

                int lastArgDelimiterIndex = findLastInstruction(argHandle, Arduino::END_ARG);
                if (lastArgDelimiterIndex > 0){
                    argHandle.removeAt(lastArgDelimiterIndex);
                }

//                qCritical() << "\nafter:\n";
//                for (int i = 0; i < argHandle.size(); ++i){
//                    qCritical() << "instr: " << std::to_string(argHandle.at(i).type).c_str();
//                }
//                qCritical() << "\n";
            }
        }

        retturn.type = Arduino::END_VAR;
        ret << retturn;
        retturn.type = Arduino::END_ST;
        ret << retturn;

        for (int i = 0; i < argHandle.size(); ++i){
            qCritical() << "arg: " << argHandle.at(i).varName << " type: " << std::to_string(argHandle.at(i).kind).c_str();
        }

        QList <Arduino::Instruction> instrs = argHandle + pre + body + post + ret;
        func.instructions = instrs.toVector().toStdVector();
        byteCode_->d.push_back(func);
    }

    QList <Arduino::Instruction> Generator::instructions(
            int modId, int algId, int level,
            const QList <AST::StatementPtr> &statements) {

        std::string types[] = {
                "Error",
                "Assign",
                "Assert",
                "VarInitialize",
                "Input",
                "Output",
                "Loop",
                "IfElse",
                "SwitchCase",
                "Break",
                "Pause",
                "Halt"};

        std::string loopTypes[] = {
                "For",
                "While",
                "Times",
                "Forever"
        };

        QList <Arduino::Instruction> result;
        for (int i = 0; i < statements.size(); i++) {
            const AST::StatementPtr st = statements[i];
            switch (st->type) {
                case AST::StError:
                    if (!st->skipErrorEvaluation)
                        ERRORR(modId, algId, level, st, result);
                    break;
                case AST::StAssign:
                    ASSIGN(modId, algId, level, st, result);
                    break;
                case AST::StAssert:
                    ASSERT(modId, algId, level, st, result);
                    break;
                case AST::StVarInitialize:
                    INIT(modId, algId, level, st, result);
                    break;
                case AST::StInput:
                    CALL_SPECIAL(modId, algId, level, st, result);
                    break;
                case AST::StOutput:
                    CALL_SPECIAL(modId, algId, level, st, result);
                    break;
                case AST::StLoop:
                    LOOP(modId, algId, level + 1, st, result);
                    break;
                case AST::StIfThenElse:
                    IFTHENELSE(modId, algId, level, st, result);
                    break;
                case AST::StSwitchCaseElse:
                    SWITCHCASEELSE(modId, algId, level, st, result);
                    break;
                case AST::StBreak:
                    BREAK(modId, algId, level, st, result);
                    break;
                case AST::StPause:
                    PAUSE_STOP(modId, algId, level, st, result);
                    break;
                case AST::StHalt:
                    PAUSE_STOP(modId, algId, level, st, result);
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

    quint16 Generator::constantValue(Arduino::ValueType type, quint8 dimension, const QVariant &value,
                                     const QString &moduleName, const QString &className) {
        QList <Arduino::ValueType> vt;
        vt.push_back(type);
        return constantValue(vt, dimension, value, QString(), QString());
    }

    void Generator::ERRORR(int, int, int, const AST::StatementPtr st, QList <Arduino::Instruction> &result) {
        const QString error = ErrorMessages::message("KumirAnalizer", QLocale::Russian, st->error);
        Arduino::Instruction e;
        e.type = Arduino::ERRORR;
        e.scope = Arduino::CONSTT;
        e.arg = constantValue(Arduino::VT_string, 0, error, QString(), QString());
        result << e;
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

    void Generator::ASSIGN(int modId, int algId, int level, const AST::StatementPtr st,
                           QList <Arduino::Instruction> &result) {
        if (st->expressions.size() > 1) {
            const AST::ExpressionPtr lvalue = st->expressions[1];
            int diff = lvalue->operands.size() - lvalue->variable->dimension;

            if (diff == 0) {
                Arduino::Instruction load;
                findVariable(modId, algId, lvalue->variable, load.scope, load.arg);
                load.type = Arduino::VAR;
                for (int i = lvalue->variable->dimension - 1; i >= 0; i--) {
                    result << calculate(modId, algId, level, lvalue->operands[i]);
                }
                load.varName = lvalue->variable->name;
                load.varType = parseVarType(lvalue->variable);
                result << load;
            }

            if (diff == 1) {
                // Set character

                result << calculate(modId, algId, level,
                                    lvalue->operands[lvalue->operands.count() - 1]);
                Arduino::Instruction argsCount;
                argsCount.type = Arduino::VAR;
                argsCount.scope = Arduino::CONSTT;
                argsCount.varType = parseVarType(lvalue->variable);
                argsCount.arg = constantValue(Arduino::VT_int, 0, 3, QString(), QString());
                result << argsCount;
            }

            if (diff == 2) {
                // Set slice

                result << calculate(modId, algId, level,
                                    lvalue->operands[lvalue->operands.count() - 2]);
                result << calculate(modId, algId, level,
                                    lvalue->operands[lvalue->operands.count() - 1]);
                Arduino::Instruction argsCount;
                argsCount.type = Arduino::ARR;
                argsCount.scope = Arduino::CONSTT;
                argsCount.varType = parseVarType(lvalue->variable);
                argsCount.arg = constantValue(Arduino::VT_int, 0, 4, QString(), QString());
                result << argsCount;
            }

            if (lvalue->kind == AST::ExprArrayElement) {
                for (int i = lvalue->variable->dimension - 1; i >= 0; i--) {
                    result << calculate(modId, algId, level, lvalue->operands[i]);
                }
            }

            Arduino::Instruction asg;
            asg.type = Arduino::ASG;
            result << asg;
        }

        const AST::ExpressionPtr rvalue = st->expressions[0];
        QList <Arduino::Instruction> rvalueInstructions = calculate(modId, algId, level, rvalue);
        Arduino::Instruction endFuncArg;
        if (st->expressions[0]->kind == AST::ExprFunctionCall) {
            for (uint16_t i = 0; i < rvalueInstructions.size(); i++) {
                Arduino::Instruction instr = rvalueInstructions.at(i);
                result << instr;
                if ((instr.type == Arduino::VAR && !instr.varType || rvalueInstructions.at(i).type == Arduino::CONST) && i + 1 < rvalueInstructions.size()) {
                    endFuncArg.type = Arduino::END_ARG;
                    result << endFuncArg;
                }
            }
        }
        else{
            result << rvalueInstructions;
        }

        Arduino::Instruction endAsg;
        endAsg.type = Arduino::END_FUNC;
        result << endAsg;
        endAsg.type = Arduino::END_VAR;
        result << endAsg;
    }

    Arduino::InstructionType parseInstructionType(AST::ExpressionPtr st) {
        switch (st->operatorr) {
            case AST::OpSumm:
                return Arduino::SUM;
            case AST::OpSubstract:
                return Arduino::SUB;
            case AST::OpMultiply:
                return Arduino::MUL;
            case AST::OpDivision:
                return Arduino::DIV;
            case AST::OpPower:
                return Arduino::POW;
            case AST::OpNot:
                return Arduino::NEG;
            case AST::OpAnd:
                return Arduino::AND;
            case AST::OpOr:
                return Arduino::OR;
            case AST::OpEqual:
                return Arduino::EQ;
            case AST::OpNotEqual:
                return Arduino::NEQ;
            case AST::OpLess:
                return Arduino::LS;
            case AST::OpGreater:
                return Arduino::GT;
            case AST::OpLessOrEqual:
                return Arduino::LEQ;
            case AST::OpGreaterOrEqual:
                return Arduino::GEQ;
            default:
                return Arduino::CONST;
        }
    }

    Arduino::ValueType parseConstType(AST::ExpressionPtr constant){
        switch (constant->constant.type()) {
            case QMetaType::Void: return Arduino::VT_void;
            case QMetaType::Bool: return Arduino::VT_bool;
            case QMetaType::Int: return Arduino::VT_int;
            case QMetaType::Float:
            case QMetaType::Double: return Arduino::VT_float;
            case QMetaType::QChar:
            case QMetaType::Char: return Arduino::VT_char;
            case QMetaType::QString: return Arduino::VT_string;
            case QMetaType::User: return Arduino::VT_struct;
            default: return Arduino::VT_void;
        }
    }

    QList<Arduino::Instruction> Generator::innerCalculation(int modId, int algId, int level, const AST::ExpressionPtr st){
        QList <Arduino::Instruction> result;
        if (st->kind == AST::ExprConst) {
            int constId = constantValue(valueType(st->baseType), st->dimension, st->constant,
                                        st->baseType.actor ? st->baseType.actor->localizedModuleName(QLocale::Russian)
                                                           : "",
                                        st->baseType.name
            );
            Arduino::Instruction instr;
            instr.scope = Arduino::CONSTT;
            instr.type = Arduino::CONST;
            instr.varType = Arduino::VT_None;
            instr.arg = constId;
            result << instr;
        } else if (st->kind == AST::ExprVariable) {
            Arduino::Instruction instr;
            instr.type = Arduino::VAR;
            instr.varName = st->variable->name;
            instr.varType = Arduino::VT_None;
//            findVariable(modId, algId, st->variable, instr.scope, instr.arg);
            result << instr;
        } else if (st->kind == AST::ExprArrayElement) {
            Arduino::Instruction instr;
            findVariable(modId, algId, st->variable, instr.scope, instr.arg);
            instr.type = Arduino::ARR;
            instr.varName = st->variable->name;
            result << instr;
            if (st->variable->dimension > 0) {
                for (int i = st->variable->dimension - 1; i >= 0; i--) {
                    result << innerCalculation(modId, algId, level, st->operands[i]);
                    qCritical() << "arr dim:" << st->operands.at(i)->constant;
                }
            }
            result << instr;
            int diff = st->operands.size() - st->variable->dimension;
            Arduino::Instruction argsCount;
            argsCount.type = Arduino::LOAD;
            argsCount.scope = Arduino::CONSTT;
            if (diff == 1) {
                // Get char
                result << innerCalculation(modId, algId, level,
                                    st->operands[st->operands.count() - 1]);
                argsCount.arg = constantValue(Arduino::VT_int, 0, 2, QString(), QString());
                result << argsCount;
            } else if (diff == 2) {
                // Get slice
                result << innerCalculation(modId, algId, level,
                                    st->operands[st->operands.count() - 2]);
                result << innerCalculation(modId, algId, level,
                                    st->operands[st->operands.count() - 1]);
                argsCount.arg = constantValue(Arduino::VT_int, 0, 3, QString(), QString());
                result << argsCount;
            }
        } else if (st->kind == AST::ExprFunctionCall) {
            const AST::AlgorithmPtr alg = st->function;

            for (int i = 0; i < alg->header.arguments.size(); i++) {
                AST::VariablePtr argument = alg->header.arguments.at(i);
                if (argument->accessType == AST::AccessArgumentOut) {
                    Arduino::Instruction instr;
                    instr.type = Arduino::VAR;
                    instr.varName = argument->name;
                    instr.varType = parseVarType(argument);
                    result << instr;
                    instr.type = Arduino::ASG;
                    result << instr;
                    break;
                }
            }

            Arduino::Instruction func;
            func.type = Arduino::FUNC;
            func.varName = alg->header.name;
            func.varType = Arduino::VT_None;
            result << func;

            for (int i = 0; i < st->operands.size(); i++) {
                AST::VariableAccessType t = alg->header.arguments[i]->accessType;
                bool arr = alg->header.arguments[i]->dimension > 0;
                if (t == AST::AccessArgumentIn && !arr) {
                    result << innerCalculation(modId, algId, level, st->operands[i]);
                } else if (t == AST::AccessArgumentIn && arr) {
                    Arduino::Instruction load;
                    load.type = Arduino::VAR;
                    findVariable(modId, algId, st->operands[i]->variable, load.scope, load.arg);
                    result << load;
                }
            }
        }
            else if (st->kind == AST::ExprSubexpression) {
            QList <Arduino::Instruction> instrs;
            QList <AST::ExpressionPtr> operands = st->operands;
            for (int i = 0; i < operands.size(); i++) {
                AST::ExpressionPtr operand = st->operands.at(i);
                if (operand->operatorr > 0 && i - 2 > 0 &&
                    (operands.at(i - 2)->operatorr == AST::OpNone && operands.at(i - 1)->operatorr == AST::OpNone)) {
                    operands.replace(i - 1, operand);
                }
            }
            for (int i = 0; i < operands.size(); i++) {
                AST::ExpressionPtr operand = operands.at(i);
                instrs.append(innerCalculation(modId, algId, level, operand));
            }
            result << instrs;
            Arduino::Instruction instr = Arduino::Instruction();
            if (st->operatorr > 0) {
                instr.type = parseInstructionType(st);
            }
            result << instr;

        }

        return result;
    }

    Arduino::Instruction Generator::parseConstOrVarExpr(AST::ExpressionPtr expr){
        Arduino::Instruction instr;
        if (expr->variable){
            instr.type = Arduino::VAR;
            instr.varName = expr->variable->name;
        } else{
            instr.type = Arduino::CONST;
            instr.arg = constantValue(Arduino::VT_string, 0, expr->constant, QString(), QString());
        }

        return instr;
    }

    QList<Arduino::Instruction> Generator::getOperands(AST::ExpressionPtr expr){
        QList<Arduino::Instruction> result;
        for (uint16_t i = 0; i < expr->operands.size(); i++){
            if (expr->operands.at(i)->operands.size() == 0){
                result << parseConstOrVarExpr(expr->operands.at(0));

                Arduino::Instruction instr;
                instr.type = parseInstructionType(expr);
                result << instr;

                result << parseConstOrVarExpr(expr->operands.at(1));
                break;
            }
            else{
                result << getOperands(expr->operands.at(i));
                if (i + 1 == expr->operands.size()){
                    break;
                }

                Arduino::Instruction instr;
                instr.type = parseInstructionType(expr);
                result << instr;
            }
        }

        return result;
    }


    QList <Arduino::Instruction> Generator::calculate(int modId, int algId, int level, const AST::ExpressionPtr st) {
        QList<Arduino::Instruction> result = innerCalculation(modId, algId, level, st);
        if (st->kind == AST::ExprSubexpression){
            result = getOperands(st);
        }

        return result;
    }

void Generator::PAUSE_STOP(int , int , int , const AST::StatementPtr  st, QList<Arduino::Instruction> & result)
{
    Arduino::Instruction a;
    a.type = st->type==AST::StPause? Arduino::PAUSE : Arduino::HALT;
    a.arg = 0u;
    result << a;
}

QList<QVariant> Generator::GetConstantValues(){
    QList<QVariant> result = QList<QVariant>();
    for (uint16_t i = 0; i < constants_.size();i++){
        result.append(constants_[i].value);
    }

    return result;
}


void Generator::ASSERT(int modId, int algId, int level, const AST::StatementPtr  st, QList<Arduino::Instruction> & result)
{
    for (int i=0; i<st->expressions.size(); i++) {
        QList<Arduino::Instruction> exprInstrs;
        exprInstrs = calculate(modId, algId, level, st->expressions[i]);
        result << exprInstrs;
        Arduino::Instruction pop;
        pop.type = Arduino::POP;
        pop.registerr = 0;
        result << pop;
        Arduino::Instruction showreg;
        showreg.type = Arduino::SHOWREG;
        showreg.registerr = pop.registerr;
        result << showreg;
        int ip = result.size(); // pointing to next of calculation (i.e. JNZ instruction)
        int targetIp = ip + 2;
        Arduino::Instruction jnz;
        jnz.type = Arduino::JNZ;
        jnz.registerr = 0;
        jnz.arg = targetIp;
        result << jnz;
        Arduino::Instruction err;
        err.type = Arduino::ERRORR;
        err.scope = Arduino::CONSTT;
        err.arg = constantValue(Arduino::VT_string, 0, tr("Assertion false"), QString(), QString());
        result << err;
    }
}

int Generator::findArrSize(QPair<QSharedPointer<AST::Expression>, QSharedPointer<AST::Expression>> bounds){
    int result = 0;
    quint16 indx = 0;

    if (!bounds.first->variable) {
        result = abs(bounds.first->constant.toInt());
    }
    else{
        AST::VariablePtr var = bounds.first->variable;
        indx = constantValue(valueType(var->baseType), var->dimension, var->initialValue,
                                 var->baseType.actor ? var->baseType.actor->localizedModuleName(QLocale::Russian) : "",
                                 var->baseType.name
        );
        result = abs(constants_[indx].value.toInt());
    }

    if (!bounds.second->variable) {
        result += abs(bounds.second->constant.toInt());
    }
    else{
        AST::VariablePtr var = bounds.second->variable;
        indx = constantValue(valueType(var->baseType), var->dimension, var->initialValue,
                                 var->baseType.actor ? var->baseType.actor->localizedModuleName(QLocale::Russian) : "",
                                 var->baseType.name
        );

        result += abs(constants_[indx].value.toInt());
    }

    return  result;
}

void Generator::INIT(int modId, int algId, int level, const AST::StatementPtr  st, QList<Arduino::Instruction> & result)
{
    for (int i=0; i<st->variables.size(); i++) {
        const AST::VariablePtr var = st->variables[i];
        Arduino::Instruction instr;
        if (var->dimension > 0 && var->bounds.size()>0) {
            instr.type = Arduino::ARR;
            instr.varName = var->name;
            instr.varType = parseVarType(var);
            result << instr;
            for (int i=var->dimension-1; i>=0 ; i--) {
                instr.varName = nullptr;
                instr.type = Arduino::CONST;
                instr.arg = std::numeric_limits<int>::max();
                instr.registerr = findArrSize(var->bounds[i]);
                result << instr;
                if (i != 0) {
                    instr.type = Arduino::END_ARG;
                    result << instr;
                }
            }
            instr.type = Arduino::END_ARR;
            result << instr;
            instr.type = Arduino::END_VAR;
            result << instr;
        }
        else if (var->dimension > 0 && var->bounds.size()==0) {
            // TODO : Implement me after compiler support
        }
        else {
            instr.type = Arduino::VAR;
            instr.varName = var->name;
            Arduino::ValueType varType = parseVarType(var);
            instr.varType = varType;
            result << instr;
            if (var->initialValue.type() != QMetaType::UnknownType) {
                instr.type = Arduino::ASG;
                result << instr;
                instr.type = Arduino::CONST;
                instr.varName = nullptr;
                qCritical() << var->name << " " << var->initialValue;
                instr.arg = constantValue(varType, 0, var->initialValue, QString(), QString());
                result << instr;
            }
            instr.type = Arduino::END_VAR;
            result << instr;
        }
    }
}

void Generator::CALL_SPECIAL(int modId, int algId, int level, const AST::StatementPtr  st, QList<Arduino::Instruction> & result)
{
    quint16 argsCount;


    if (st->type==AST::StOutput) {
        int varsCount = st->expressions.size() / 3;
        Arduino::Instruction output;
        output.type = Arduino::OUTPUT;
        result << output;
        for (int i = 0; i<varsCount; ++i) {
            const AST::ExpressionPtr  expr = st->expressions[3*i];
            result << calculate(modId, algId, level, expr);

            if (i + 1 < varsCount) {
                output.type = Arduino::SUM;
                result << output;
            }
        }

        if (st->expressions.size() % 3) {
            // File handle
            QList<Arduino::Instruction> instrs = calculate(modId, algId, level, st->expressions.last());
            result << instrs;
        }

        argsCount = st->expressions.size();
    }
    else if (st->type==AST::StInput) {
        Arduino::Instruction input;
        input.type = Arduino::INPUT;
        result << input;

        int varsCount = st->expressions.size();
        for (int i = varsCount-1; i>=0; i--) {
            const AST::ExpressionPtr  varExpr = st->expressions[i];
            Arduino::Instruction ref;
            if (varExpr->kind==AST::ExprConst) {
                ref.scope = Arduino::CONSTT;
                ref.arg = constantValue(valueType(varExpr->baseType), 0, varExpr->constant,
                                        varExpr->baseType.actor ? varExpr->baseType.actor->localizedModuleName(QLocale::Russian) : "",
                                        varExpr->baseType.name
                                        );
            }
            else {
                findVariable(modId, algId, varExpr->variable, ref.scope, ref.arg);
            }
            if (varExpr->kind==AST::ExprVariable || varExpr->kind==AST::ExprConst) {
                ref.type = Arduino::REF;
                result << ref;
            }
            else if (varExpr->kind == AST::ExprArrayElement) {
                ref.type = Arduino::REFARR;
                for (int j=varExpr->operands.size()-1; j>=0; j--) {
                    QList<Arduino::Instruction> operandInstrs = calculate(modId, algId, level, varExpr->operands[j]);
                    result << operandInstrs;
                }
                result << ref;
            }
            else {
                QList<Arduino::Instruction> operandInstrs = calculate(modId, algId, level, varExpr);
                result << operandInstrs;
            }

        }
        argsCount = st->expressions.size();
    }

    Arduino::Instruction endCall;
    endCall.type = Arduino::END_VAR;
    result << endCall;
}


void Generator::IFTHENELSE(int modId, int algId, int level, const AST::StatementPtr  st, QList<Arduino::Instruction> &result)
{
    int jzIP = -1;
    if (st->conditionals[0].condition) {
        QList<Arduino::Instruction> conditionInstructions = calculate(modId, algId, level, st->conditionals[0].condition);
        result << conditionInstructions;

        jzIP = result.size();
        Arduino::Instruction jz;
        jz.type = Arduino::JZ;
        jz.registerr = 0;
        result << jz;
    }


    Arduino::Instruction error;
    if (st->conditionals[0].conditionError.size()>0) {
        const QString msg = ErrorMessages::message("KumirAnalizer", QLocale::Russian, st->conditionals[0].conditionError);
        error.type = Arduino::ERRORR;
        error.scope = Arduino::CONSTT;
        error.arg = constantValue(Arduino::VT_string, 0, msg, QString(), QString());
        result << error;
    }
    else {
        QList<Arduino::Instruction> thenInstrs = instructions(modId, algId, level, st->conditionals[0].body);
        result += thenInstrs;
    }

    if (jzIP!=-1)
        result[jzIP].arg = result.size();

    if (st->conditionals.size()>1) {
        int jumpIp = result.size();
        Arduino::Instruction jump;
        jump.type = Arduino::JUMP;
        result << jump;
        result[jzIP].arg = result.size();
        if (st->conditionals[1].conditionError.size()>0) {
            const QString msg = ErrorMessages::message("KumirAnalizer", QLocale::Russian, st->conditionals[1].conditionError);
            error.type = Arduino::ERRORR;
            error.scope = Arduino::CONSTT;
            error.arg = constantValue(Arduino::VT_string, 0, msg, QString(), QString());
            result << error;
        }
        else {
            QList<Arduino::Instruction> elseInstrs = instructions(modId, algId, level, st->conditionals[1].body);
            result += elseInstrs;
        }
        result[jumpIp].arg = result.size();
    }

    if (st->endBlockError.size()>0) {
        const QString msg = ErrorMessages::message("KumirAnalizer", QLocale::Russian, st->endBlockError);
        error.type = Arduino::ERRORR;
        error.scope = Arduino::CONSTT;
        error.arg = constantValue(Arduino::VT_string, 0, msg, QString(), QString());
        result << error;
    }

}

void Generator::SWITCHCASEELSE(int modId, int algId, int level, const AST::StatementPtr st, QList<Arduino::Instruction> & result)
{
    if (st->headerError.size()>0) {
        Arduino::Instruction garbage;
        garbage.type = Arduino::ERRORR;
        garbage.scope = Arduino::CONSTT;
        garbage.arg = constantValue(Arduino::VT_string, 0,
                                    ErrorMessages::message("KumirAnalizer", QLocale::Russian, st->headerError)
                                    , QString(), QString()
                                    );
        result << garbage;
        return;
    }

    if (st->beginBlockError.size()>0) {
        const QString error = ErrorMessages::message("KumirAnalizer", QLocale::Russian, st->beginBlockError);
        Arduino::Instruction err;
        err.type = Arduino::ERRORR;
        err.scope = Arduino::CONSTT;
        err.arg = constantValue(Arduino::VT_string, 0, error, QString(), QString());
        result << err;
        return;
    }

    int lastJzIp = -1;
    QList<int> jumpIps;



    for (int i=0; i<st->conditionals.size(); i++) {
        if (lastJzIp!=-1) {
            result[lastJzIp].arg = result.size();
            lastJzIp = -1;
        }
        if (!st->conditionals[i].conditionError.isEmpty()) {
            const QString error = ErrorMessages::message("KumirAnalizer", QLocale::Russian, st->conditionals[i].conditionError);
            Arduino::Instruction err;
            err.type = Arduino::ERRORR;
            err.scope = Arduino::CONSTT;
            err.arg = constantValue(Arduino::VT_string, 0, error, QString(), QString());
            result << err;
        }
        else {
            if (st->conditionals[i].condition) {
                QList<Arduino::Instruction> condInstrs = calculate(modId, algId, level, st->conditionals[i].condition);
                result << condInstrs;
                Arduino::Instruction pop;
                pop.type = Arduino::POP;
                pop.registerr = 0;
                result << pop;
                Arduino::Instruction showreg;
                showreg.type = Arduino::SHOWREG;
                showreg.registerr = 0;
                result << showreg;
                Arduino::Instruction jz;
                jz.type = Arduino::JZ;
                jz.registerr = 0;
                lastJzIp = result.size();
                result << jz;
            }
            QList<Arduino::Instruction> instrs = instructions(modId, algId, level, st->conditionals[i].body);
            result += instrs;
            if (i<st->conditionals.size()-1) {
                Arduino::Instruction jump;
                jump.type = Arduino::JUMP;
                jumpIps << result.size();
                result << jump;
            }
        }
    }
    if (lastJzIp!=-1)
        result[lastJzIp].arg = result.size();
    for (int i=0; i<jumpIps.size(); i++) {
        result[jumpIps[i]].arg = result.size();
    }
}

const AST::VariablePtr  Generator::returnValue(const AST::AlgorithmPtr  alg)
{
    const QString name = alg->header.name;
    for (int i=0; i<alg->impl.locals.size(); i++) {
        if (alg->impl.locals[i]->name == name) {
            qCritical() << "1231";
            return alg->impl.locals[i];
        }
    }
    return nullptr;
}

void Generator::BREAK(int , int , int level,
                      const AST::StatementPtr  st,
                      QList<Arduino::Instruction> & result)
{
    Arduino::Instruction br;
    br.type = Arduino::BREAK;

    result << br;
}

Arduino::InstructionType getForLoopValuesRelation(int fromValue, int toValue){
    if (fromValue > toValue){
        return Arduino::GT;
    } else if (fromValue >= toValue){
        return Arduino::GEQ;
    } else if (fromValue < toValue){
        return Arduino::LS;
    } else if (fromValue <= toValue){
        return Arduino::LEQ;
    }

    return Arduino::NEQ;
}

Arduino::Instruction getForLoopVariable(AST::ExpressionPtr & ex) {
        Arduino::Instruction variable;

        if (ex->variable == nullptr) {
            variable.registerr = ex->constant.toInt();
            variable.type = Arduino::CONST;
        } else {
            variable.varName = ex->variable->name;
            variable.registerr = ex->variable->initialValue.toInt();
            variable.type = Arduino::VAR;
        }

        return variable;
    }

void addForLoopStep(AST::ExpressionPtr & ex, QList<Arduino::Instruction> &instructions, int fromValue, int toValue) {
        Arduino::Instruction step;

        if(ex == nullptr){
            if (fromValue > toValue){
                step.type = Arduino::DCR;
            } else {
                step.type = Arduino::INC;
            }
            instructions << step;

            return;
        }
        if (ex->variable == nullptr){
            step.registerr = ex->constant.toInt();
        }
        else{
            step.registerr = ex->variable->initialValue.toInt();
        }

        if (step.registerr > 0){
            if (abs(step.registerr) == 1){
                step.type = Arduino::INC;
            } else {
                step.type = Arduino::SUM;
            }
        } else {
            if (abs(step.registerr) == 1){
                step.type = Arduino::DCR;
            } else {
                step.type = Arduino::SUB;
            }
        }

        instructions << step;
    }

void Generator::LOOP(int modId, int algId,
                     int level,
                     const AST::StatementPtr st,
                     QList<Arduino::Instruction> &result) {
    if (st->beginBlockError.size() > 0) {
        const QString error = ErrorMessages::message("KumirAnalizer", QLocale::Russian, st->beginBlockError);
        Arduino::Instruction err;
        err.type = Arduino::ERRORR;
        err.scope = Arduino::CONSTT;
        err.arg = constantValue(Arduino::VT_string, 0, error, QString(), QString());
        result << err;
        return;
    }

    int beginIp = result.size();
    int jzIp = -1;


    if (st->loop.type == AST::LoopWhile || st->loop.type == AST::LoopForever) {
        // Highlight line and clear margin
        if (st->loop.whileCondition) {
            Arduino::Instruction instr;
            instr.type = Arduino::WhileLoop;
            result << instr;

            QList <Arduino::Instruction> whileCondInstructions = calculate(modId, algId, level,
                                                                           st->loop.whileCondition);

            result << whileCondInstructions;
        }
    } else if (st->loop.type == AST::LoopTimes) {
        Arduino::Instruction instrBuffer;
        Arduino::Instruction timesVar;

        instrBuffer.type = Arduino::WhileLoop;
        result << instrBuffer;

        timesVar.type = Arduino::VAR;
        timesVar.varName = st->loop.timesValue->variable->name;
        result << timesVar;

        instrBuffer.type = Arduino::GT;
        result << instrBuffer;

        instrBuffer.type = Arduino::CONST;
        instrBuffer.registerr = 0;
        result << instrBuffer;

    } else if (st->loop.type == AST::LoopFor) {
        Arduino::Instruction instrBuffer;
        Arduino::Instruction workCondition;
        Arduino::Instruction operation;

        instrBuffer.type = Arduino::ForLoop;
        result << instrBuffer;

        workCondition.type = Arduino::VAR;
        workCondition.varName = st->loop.forVariable->name;
        result << workCondition;

        operation.type = Arduino::ASG;
        result << operation;

        Arduino::Instruction fromValue = getForLoopVariable(st->loop.fromValue);
        result << fromValue;

        workCondition.type = Arduino::END_VAR;
        result << workCondition;

        workCondition.type = Arduino::VAR;
        result << workCondition;

        Arduino::Instruction toValue = getForLoopVariable(st->loop.toValue);

        operation.type = getForLoopValuesRelation(fromValue.registerr, toValue.registerr);
        result << operation;

        result << toValue;

        workCondition.type = Arduino::END_VAR;
        result << workCondition;

        workCondition.type = Arduino::VAR;
        result << workCondition;

        addForLoopStep(st->loop.stepValue, result, result.at(result.size() - 2).registerr,
                       result.at(result.size() - 1).registerr);
    }

    Arduino::Instruction endLoopHead;
    endLoopHead.type = Arduino::END_ST_HEAD;
    result << endLoopHead;

    QList <Arduino::Instruction> instrs = instructions(modId, algId, level, st->loop.body);
    result += instrs;

    bool endsWithError = st->endBlockError.length() > 0;
    if (endsWithError) {
        const QString error = ErrorMessages::message("KumirAnalizer", QLocale::Russian, st->endBlockError);
        Arduino::Instruction ee;
        ee.type = Arduino::ERRORR;
        ee.scope = Arduino::CONSTT;
        ee.arg = constantValue(Arduino::VT_string, 0, error, QString(), QString());
        result << ee;
        return;
    }

    if (st->loop.endCondition) {
        QList <Arduino::Instruction> endCondInstructions = calculate(modId, algId, level, st->loop.endCondition);
        result << endCondInstructions;
    }

    Arduino::Instruction endLoop;
    endLoop.type = Arduino::END_ST;
    result << endLoop;
}

} // namespace ArduinoCodeGenerator
