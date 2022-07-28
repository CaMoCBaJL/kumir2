#include "generator.h"
#include "arduino_tableelem.hpp"
#include <kumir2-libs/extensionsystem/pluginmanager.h>
#include <kumir2/actorinterface.h>
#include <kumir2-libs/dataformats/lexem.h>
#include <kumir2-libs/stdlib/kumirstdlib.hpp>

#include "arduino_data.hpp"
#include "entities/CVariable.h"
#include <deque>
#include <cmath>
#include <limits>
#include "enums/enums.h"
#include "entities/Instruction.h"
#include "entities/TableElem.h"
#include "entities/Data.h"
#include "generators/LoopGenerator/LoopGenerator.hpp"
#include "entities/ConstValue.h"
#include "generators/CommonMethodsProvider.hpp"
#include "generators/ExpressionCalculator.hpp"

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
            varsToOut << constantValue(Arduino::VT_int, 0, 0, QString(), QString(), this->constants_);
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
                    instrs << initBounds;
                }
                Arduino::Instruction bounds;
                bounds.type = Arduino::ARR;
                bounds.scope = Arduino::LOCAL;
                bounds.arg = quint16(i + locOffset);
                instrs << bounds;
            }

        }

        Arduino::Instruction callInstr;
        callInstr.type = Arduino::CALL;
        findFunction(alg, callInstr.module, callInstr.arg);
        instrs << callInstr;

        Arduino::TableElem func;
        func.type = Arduino::EL_BELOWMAIN;
        func.algId = func.id = algId;
        func.module = moduleId;
        func.moduleLocalizedName = mod->header.name.toStdWString();
        func.name = QString::fromLatin1("@below_main").toStdWString();
        func.instructions = instrs.toVector().toStdVector();
        byteCode_->d.push_back(func);

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

        Arduino::TableElem func;
        func.type = type;
        func.module = moduleId;
        func.algId = func.id = id;
        std::string algName = alg->header.name.toStdString();
        func.name = std::wstring(algName.begin(), algName.end());
        func.moduleLocalizedName = mod->header.name.toStdWString();
        QList <Arduino::Instruction> argHandle;

        for (int i = 0; i < alg->header.arguments.size(); i++) {
            const AST::VariablePtr var = alg->header.arguments[i];
            if (var->dimension > 0) {
                for (int i = var->dimension - 1; i >= 0; i--) {
                    argHandle << calculate(moduleId, id, 0, var->bounds[i].second);
                    argHandle << calculate(moduleId, id, 0, var->bounds[i].first);
                }
                Arduino::Instruction bounds;
                bounds.type = Arduino::VAR;
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
            err.arg = constantValue(Arduino::VT_string, 0, beginError, QString(), QString(), this->constants_);
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
                err.arg = constantValue(Arduino::VT_string, 0, endError, QString(), QString(), this->constants_);
                ret << err;
            }
        }

        Arduino::Instruction retturn;
        retturn.type = Arduino::RET;
        ret << retturn;

        const AST::VariablePtr retval = returnValue(alg);
        if (retval != nullptr) {
            Arduino::Instruction delim;
            delim.type = Arduino::STR_DELIM;
            ret.push_front(delim);

            Arduino::Instruction loadRetval;
            loadRetval.type = Arduino::VAR;
            loadRetval.varType = Arduino::VT_None;
            loadRetval.varName = retval->name;
            ret << loadRetval;

            loadRetval.varName = QString::null;
            loadRetval.varType = parseVarType(retval);
            argHandle << loadRetval;
        } else {
            int lastArgIndex = findLastInstruction(argHandle, Arduino::VAR);
            if (lastArgIndex > -1 && argHandle.at(lastArgIndex).kind > 1) {
                Arduino::Instruction funcResult;
                funcResult.type = Arduino::VAR;
                funcResult.varName = argHandle.at(lastArgIndex).varName;
                ret << funcResult;
                Arduino::Instruction retArg = argHandle.at(lastArgIndex);
                retArg.varName = QString::null;

                //insert declaration of return var in the begining of func body
                //insert order reversed(str_delim -> end_var -> var), because each time
                //we insert instruction in the beginning of body instruction list
                //first will move to second position
                funcResult.type = Arduino::END_VAR;
                body.push_front(funcResult);
                body.push_front(argHandle.at(lastArgIndex));

                argHandle.removeAt(lastArgIndex);
                argHandle.insert(lastArgIndex, retArg);

                int lastArgDelimiterIndex = findLastInstruction(argHandle, Arduino::END_ARG);
                if (lastArgDelimiterIndex > 0){
                    argHandle.removeAt(lastArgDelimiterIndex);
                }
            } else {
                Arduino::Instruction retArg;
                retArg.type = Arduino::VAR;
                retArg.varType = Arduino::VT_void;

                argHandle.push_back(retArg);
            }
        }

        int returnValueIndex;

        for (int i = argHandle.size() - 1; i > 0; --i){
            if (argHandle.at(i).type == Arduino::VAR){
                if (argHandle.at(i).varName == QString::null) {
                    returnValueIndex = i;
                } else {
                    returnValueIndex = -1;
                }
                break;
            }
        }

        if ((returnValueIndex > 0 || retval != nullptr) && pre.size() > 0){
            Arduino::Instruction endPre;
            endPre.type = Arduino::END_ST;
            ret.push_front(endPre);
        }

        retturn.type = Arduino::END_VAR;
        ret << retturn;
        retturn.type = Arduino::END_ST;
        ret << retturn;

        QList <Arduino::Instruction> instrs = argHandle + pre + body + post + ret;
        func.instructions = instrs.toVector().toStdVector();
        byteCode_->d.push_back(func);
    }

    void Generator::ERRORR(int, int, int, const AST::StatementPtr st, QList <Arduino::Instruction> &result) {
        const QString error = ErrorMessages::message("KumirAnalizer", QLocale::Russian, st->error);
        Arduino::Instruction e;
        e.type = Arduino::ERRORR;
        e.scope = Arduino::CONSTT;
        e.arg = constantValue(Arduino::VT_string, 0, error, QString(), QString(), this->constants_);
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
                Arduino::Instruction lVar;
                findVariable(modId, algId, lvalue->variable, lVar.scope, lVar.arg);
                lVar.type = Arduino::VAR;

                for (int i = lvalue->variable->dimension - 1; i >= 0; i--) {
                    result << calculate(modId, algId, level, lvalue->operands[i]);
                }

                lVar.varName = lvalue->variable->name;
                lVar.varType = Arduino::VT_None;

                result << lVar;
            }

            if (diff == 1) {
                // Set character

                result << calculate(modId, algId, level,
                                    lvalue->operands[lvalue->operands.count() - 1]);
                Arduino::Instruction argsCount;
                argsCount.type = Arduino::VAR;
                argsCount.scope = Arduino::CONSTT;
                argsCount.varType = parseVarType(lvalue->variable);
                argsCount.arg = constantValue(Arduino::VT_int, 0, 3, QString(), QString(), this->constants_);
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
                argsCount.arg = constantValue(Arduino::VT_int, 0, 4, QString(), QString(), this->constants_);
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
        result << rvalueInstructions;

        Arduino::Instruction endAsg;
        if (rvalue->kind == AST::ExprFunctionCall) {
            endAsg.type = Arduino::END_SUB_EXPR;
            result << endAsg;
        }
        endAsg.type = Arduino::END_VAR;
        result << endAsg;
    }

void Generator::PAUSE_STOP(int , int , int , const AST::StatementPtr  st, QList<Arduino::Instruction> & result)
{
    Arduino::Instruction a;
    a.type = Arduino::BREAK;
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

void Generator::ASSERT(int modId, int algId, int level, const AST::StatementPtr  st, QList<Arduino::Instruction> & result) {
    if (st->expressions.size() <= 0) {
        return;
    }

    Arduino::Instruction ifInstr;
    ifInstr.type = Arduino::IF;
    result << ifInstr;

    ifInstr.type = Arduino::START_SUB_EXPR;
    result << ifInstr;

    ifInstr.type = Arduino::NEG;
    result << ifInstr;

    ifInstr.type = Arduino::START_SUB_EXPR;
    result << ifInstr;

    for (int i = 0; i < st->expressions.size(); i++) {
        QList <Arduino::Instruction> exprInstrs;
        exprInstrs = calculate(modId, algId, level, st->expressions[i]);
        result << exprInstrs;
    }

    ifInstr.type = Arduino::END_SUB_EXPR;
    result << ifInstr;

    ifInstr.type = Arduino::END_ST_HEAD;
    result << ifInstr;
}

int Generator::findArrSize(QPair<QSharedPointer<AST::Expression>, QSharedPointer<AST::Expression>> bounds){
    int result = 0;
    quint16 indx = 0;

    if (!bounds.first->variable) {
        result = bounds.first->constant.toInt();
    }
    else{
        AST::VariablePtr var = bounds.first->variable;
        indx = constantValue(valueType(var->baseType), var->dimension, var->initialValue,
                                 var->baseType.actor ? var->baseType.actor->localizedModuleName(QLocale::Russian) : "",
                                 var->baseType.name, this->constants_
        );
        result = constants_[indx].value.toInt();
    }

    if (!bounds.second->variable) {
        result = abs(result - bounds.second->constant.toInt());
    }
    else{
        AST::VariablePtr var = bounds.second->variable;
        indx = constantValue(valueType(var->baseType), var->dimension, var->initialValue,
                                 var->baseType.actor ? var->baseType.actor->localizedModuleName(QLocale::Russian) : "",
                                 var->baseType.name, this->constants_
        );

        result = abs(result - constants_[indx].value.toInt());
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
            for (int i=0; i< var->dimension; i++) {
                instr.varName = nullptr;
                instr.type = Arduino::CONST;
                instr.arg = std::numeric_limits<int>::max();
                instr.registerr = findArrSize(var->bounds[i]);
                result << instr;
                if (i + 1 < var->dimension) {
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
                instr.arg = constantValue(varType, 0, var->initialValue, QString(), QString(), this->constants_);
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
                                        varExpr->baseType.name, this->constants_
                                        );
            }
            else {
                findVariable(modId, algId, varExpr->variable, ref.scope, ref.arg);
            }
            if (varExpr->kind==AST::ExprVariable || varExpr->kind==AST::ExprConst) {
                ref.type = Arduino::CONST;
                result << ref;
            }
            else if (varExpr->kind == AST::ExprArrayElement) {
                ref.type = Arduino::ARR;
                for (int j=varExpr->operands.size()-1; j>=0; j--) {
                    QList<Arduino::Instruction> operandInstrs = calculate(modId, algId, level, varExpr->operands[j]);
                    result << operandInstrs;
                }
                result << ref;

                ref.type = Arduino::END_ARR;
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
    Arduino::Instruction ifInstr;
    ifInstr.type = Arduino::IF;
    result << ifInstr;

    ifInstr.type = Arduino::START_SUB_EXPR;
    result << ifInstr;

    if (st->conditionals[0].condition) {
        QList<Arduino::Instruction> conditionInstructions = calculate(modId, algId, level, st->conditionals[0].condition);
        result << conditionInstructions;
        ifInstr.type = Arduino::END_ST_HEAD;
        result << ifInstr;
    }

    Arduino::Instruction error;
    if (st->conditionals[0].conditionError.size()>0) {
        const QString msg = ErrorMessages::message("KumirAnalizer", QLocale::Russian, st->conditionals[0].conditionError);
        error.type = Arduino::ERRORR;
        error.scope = Arduino::CONSTT;
        error.arg = constantValue(Arduino::VT_string, 0, msg, QString(), QString(), this->constants_);
        result << error;
    }
    else {
        QList<Arduino::Instruction> thenInstrs = instructions(modId, algId, level, st->conditionals[0].body);
        result += thenInstrs;
        ifInstr.type = Arduino::END_ST;
        result << ifInstr;
    }

    if (st->conditionals.size()>1) {
        if (st->conditionals[1].conditionError.size()>0) {
            const QString msg = ErrorMessages::message("KumirAnalizer", QLocale::Russian, st->conditionals[1].conditionError);
            error.type = Arduino::ERRORR;
            error.scope = Arduino::CONSTT;
            error.arg = constantValue(Arduino::VT_string, 0, msg, QString(), QString(), this->constants_);
            result << error;
        }
        else {
            ifInstr.type = Arduino::ELSE;
            result << ifInstr;
            QList<Arduino::Instruction> elseInstrs = instructions(modId, algId, level, st->conditionals[1].body);
            result += elseInstrs;
            ifInstr.type = Arduino::END_ST;
            result << ifInstr;
        }
    }

    if (st->endBlockError.size()>0) {
        const QString msg = ErrorMessages::message("KumirAnalizer", QLocale::Russian, st->endBlockError);
        error.type = Arduino::ERRORR;
        error.scope = Arduino::CONSTT;
        error.arg = constantValue(Arduino::VT_string, 0, msg, QString(), QString(), this->constants_);
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
                                    , QString(), QString(), this->constants_
                                    );
        result << garbage;
        return;
    }

    if (st->beginBlockError.size()>0) {
        const QString error = ErrorMessages::message("KumirAnalizer", QLocale::Russian, st->beginBlockError);
        Arduino::Instruction err;
        err.type = Arduino::ERRORR;
        err.scope = Arduino::CONSTT;
        err.arg = constantValue(Arduino::VT_string, 0, error, QString(), QString(), this->constants_);
        result << err;
        return;
    }

    for (int i=0; i<st->conditionals.size(); i++) {
        Arduino::Instruction ifInstr;
        if (i > 0){
            ifInstr.type = Arduino::ELSE;
            result << ifInstr;
        }

        if (i + 1 != st->conditionals.size()) {
            ifInstr.type = Arduino::IF;
            result << ifInstr;

            ifInstr.type = Arduino::START_SUB_EXPR;
            result << ifInstr;
        }
        if (!st->conditionals[i].conditionError.isEmpty()) {
            const QString error = ErrorMessages::message("KumirAnalizer", QLocale::Russian, st->conditionals[i].conditionError);
            Arduino::Instruction err;
            err.type = Arduino::ERRORR;
            err.scope = Arduino::CONSTT;
            err.arg = constantValue(Arduino::VT_string, 0, error, QString(), QString(), this->constants_);
            result << err;
        }
        else {
            if (st->conditionals[i].condition) {
                QList<Arduino::Instruction> condInstrs = calculate(modId, algId, level, st->conditionals[i].condition);
                result << condInstrs;

                ifInstr.type = Arduino::END_ST_HEAD;
                result << ifInstr;
            }
            QList<Arduino::Instruction> instrs = instructions(modId, algId, level, st->conditionals[i].body);
            result += instrs;

            ifInstr.type = Arduino::END_ST;
            result << ifInstr;
        }
    }
}

const AST::VariablePtr  Generator::returnValue(const AST::AlgorithmPtr  alg)
{
    const QString name = alg->header.name;
    for (int i=0; i<alg->impl.locals.size(); i++) {
        if (alg->impl.locals[i]->name == name) {
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

} // namespace ArduinoCodeGenerator
