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
            case Arduino::VT_real:
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
            case Arduino::VT_record: {
                QVariantList valueFields = val.toList();
                Arduino::Record record;
                for (int i = 1; i < vt.size(); i++) {
                    Arduino::ValueType fieldType = vt.at(i);
                    switch (fieldType) {
                        case Arduino::VT_int:
                            record.fields.push_back(Arduino::CAnyValue(valueFields[i - 1].toInt()));
                            break;
                        case Arduino::VT_real:
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

    static Arduino::TableElem makeConstant(const ConstValue &val) {
        Arduino::TableElem e;
        e.type = Arduino::EL_CONST;
        e.vtype = val.baseType.toStdList();
        e.dimension = val.dimension;
        e.recordModuleLocalizedName = val.recordModuleName.toStdWString();
        e.recordClassLocalizedName = val.recordClassLocalizedName.toStdWString();
        if (val.dimension == 0) {
            Arduino::CVariable var;
            Arduino::CAnyValue vv = makeAnyValue(val.value,
                                                 val.baseType,
                                                 val.recordModuleName,
                                                 val.recordClassLocalizedName
            );
            var.setValue(vv);
            var.setBaseType(val.baseType.front());
            var.setDimension(val.dimension);
            var.setConstantFlag(true);
            e.initialValue = var;
        } else {
            int sizes[3];
            getVarListSizes(val.value, sizes, 0);
            Arduino::CVariable var;
            var.setConstantFlag(true);
            var.setBaseType(val.baseType.front());
            var.setDimension(val.dimension);
            int bounds[7] = {1, sizes[0], 1, sizes[1], 1, sizes[2], var.dimension() * 2};
            var.setBounds(bounds);
            var.init();
            if (var.dimension() == 1) {
                QVariantList listX = val.value.toList();
                for (int x = 1; x <= sizes[0]; x++) {
                    if (x - 1 < listX.size()) {
                        var.setValue(x, makeAnyValue(
                                listX[x - 1],
                                val.baseType,
                                val.recordModuleName,
                                val.recordClassLocalizedName
                        ));
                    } else {
                        var.setValue(x, Arduino::CAnyValue());
                    }
                }
            } // end if (var.dimension()==1)
            else if (var.dimension() == 2) {
                QVariantList listY = val.value.toList();
                for (int y = 1; y <= sizes[0]; y++) {
                    if (y - 1 < listY.size()) {
                        QVariantList listX = listY[y - 1].toList();
                        for (int x = 1; x <= sizes[1]; x++) {
                            if (x - 1 < listX.size()) {
                                var.setValue(y, x, makeAnyValue(
                                        listX[x - 1],
                                        val.baseType,
                                        val.recordModuleName,
                                        val.recordClassLocalizedName
                                ));
                            } else {
                                var.setValue(y, x, Arduino::CAnyValue());
                            }
                        }
                    } else {
                        for (int x = 1; x <= sizes[1]; x++) {
                            var.setValue(y, x, Arduino::CAnyValue());
                        }
                    }
                }
            } // end else if (var.dimension()==2)
            else if (var.dimension() == 3) {
                QVariantList listZ = val.value.toList();
                for (int z = 1; z <= sizes[0]; z++) {
                    if (z - 1 < listZ.size()) {
                        QVariantList listY = listZ[z - 1].toList();
                        for (int y = 1; y <= sizes[1]; y++) {
                            if (y - 1 < listY.size()) {
                                QVariantList listX = listY[y - 1].toList();
                                for (int x = 1; x <= sizes[2]; x++) {
                                    if (x - 1 < listX.size()) {
                                        var.setValue(z, y, x, makeAnyValue(
                                                listX[x - 1],
                                                val.baseType,
                                                val.recordModuleName,
                                                val.recordClassLocalizedName
                                        ));
                                    } else {
                                        var.setValue(z, y, x, Arduino::CAnyValue());
                                    }
                                }
                            } else {
                                for (int x = 1; x <= sizes[1]; x++) {
                                    var.setValue(z, y, x, Arduino::CAnyValue());
                                }
                            }
                        }
                    } else {
                        for (int y = 1; y <= sizes[1]; y++) {
                            for (int x = 1; x <= sizes[2]; x++) {
                                var.setValue(z, y, x, Arduino::CAnyValue());
                            }
                        }
                    }
                }
            } // end else if (var.dimension()==2)
            e.initialValue = var;
        }
        return e;
    }

    Arduino::TableElem Generator::AddConstName(Arduino::Data &data, Kumir::String constName, uint16_t constId) {
        for (size_t i = 0; i < data.d.size(); i++) {
            if (data.d.at(i).id == constId) {
                Arduino::TableElem e = Arduino::TableElem(data.d.at(i));
                e.name = constName;
                return e;
            }
        }

        return Arduino::TableElem();
    }

    bool
    isElementAlreadyAdded(const Arduino::TableElem elemFromSource, const std::deque <Arduino::TableElem> newElements) {
        for (size_t j = 0; j < newElements.size(); j++) {
            if (newElements.at(j).id == elemFromSource.id) {
                return true;
            }
        }

        return false;
    }

    void Generator::UpdateConstants(Arduino::Data &data) {
        std::deque <Arduino::TableElem> newConstants;
        for (size_t i = data.d.size(); i > 0; i--) {
            Arduino::TableElem elem = data.d.at(i - 1);
            switch (elem.type) {
                case Arduino::EL_GLOBAL:
                case Arduino::EL_LOCAL:
                    if (!(elem.name.empty())) {
                        qCritical() << "elem type" << std::to_string(elem.type).c_str();
                        newConstants.push_back(AddConstName(data, elem.name, elem.id));
                    }
                    break;
                default:
                    newConstants.push_back(elem);
                    break;
            }
        }
        std::reverse(newConstants.begin(), newConstants.end());
        data.d.swap(newConstants);
    }

    void Generator::generateConstantTable() {
        for (int i = constants_.size() - 1; i >= 0; i--) {
            const ConstValue cons = constants_[i];
            Arduino::TableElem e = makeConstant(cons);
            e.id = i;
            byteCode_->d.push_front(e);
        }
    }

    static const Shared::ActorInterface::Function &
    functionByInternalId(const Shared::ActorInterface *actor, uint32_t id) {
        static Shared::ActorInterface::Function dummy;
        const Shared::ActorInterface::FunctionList &alist = actor->functionList();
        foreach(
        const Shared::ActorInterface::Function &func, alist) {
            if (func.id == id) {
                return func;
            }
        }
        return dummy;
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
            result << Arduino::VT_real;
        else if (t.kind == AST::TypeBoolean)
            result << Arduino::VT_bool;
        else if (t.kind == AST::TypeString)
            result << Arduino::VT_string;
        else if (t.kind == AST::TypeCharect)
            result << Arduino::VT_char;
        else if (t.kind == AST::TypeUser) {
            result << Arduino::VT_record;
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

    void Generator::shiftInstructions(QList <Arduino::Instruction> &instrs, int offset) {
        for (int i = 0; i < instrs.size(); i++) {
            Arduino::InstructionType t = instrs.at(i).type;
            if (t == Arduino::JNZ || t == Arduino::JZ || t == Arduino::JUMP) {
                instrs[i].arg += offset;
            }
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
            Arduino::Instruction init;
            init.type = Arduino::INIT;
            init.scope = Arduino::LOCAL;
            init.arg = quint16(i + locOffset);
            instrs << init;
            if (var->initialValue.isValid() && var->dimension == 0) {
                Arduino::Instruction load;
                load.type = Arduino::LOAD;
                load.scope = Arduino::CONSTT;
                load.arg = constantValue(valueType(var->baseType),
                                         0,
                                         var->initialValue,
                                         var->baseType.actor ?
                                         var->baseType.actor->localizedModuleName(QLocale::Russian) : "",
                                         var->baseType.name
                );
                instrs << load;
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
                call.module = 0xFF;
                call.arg = 0xBB01;

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
            callShow.module = 0xFF;
            callShow.arg = 0xBB02;
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

        for (int i = 0; i < alg->impl.locals.size(); i++) {
            const AST::VariablePtr var = alg->impl.locals[i];
            Arduino::TableElem loc;
            loc.type = Arduino::EL_LOCAL;
            loc.module = moduleId;
            loc.algId = id;
            loc.id = i;
            loc.name = var->name.toStdWString();
            loc.dimension = var->dimension;
            loc.vtype = valueType(var->baseType).toStdList();
            loc.refvalue = valueKind(var->accessType);
            loc.recordModuleAsciiName = var->baseType.actor ?
                                        std::string(var->baseType.actor->asciiModuleName().constData()) : std::string();
            loc.recordModuleLocalizedName = var->baseType.actor ?
                                            var->baseType.actor->localizedModuleName(QLocale::Russian).toStdWString()
                                                                : std::wstring();
            loc.recordClassAsciiName = std::string(var->baseType.asciiName);
            loc.recordClassLocalizedName = var->baseType.name.toStdWString();
            byteCode_->d.push_back(loc);
        }
        Arduino::TableElem func;
        func.type = type;
        func.module = moduleId;
        func.algId = func.id = id;
        func.name = alg->header.name.toStdWString();
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

        if (alg->impl.endLexems.size() > 0 && debugLevel_ == GeneratorInterface::LinesAndVariables) {

        }


        for (int i = alg->header.arguments.size() - 1; i >= 0; i--) {
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
            }
            if (var->accessType == AST::AccessArgumentOut) {
                Arduino::Instruction init;
                init.type = Arduino::INIT;
                findVariable(moduleId, id, var, init.scope, init.arg);
                argHandle << init;
            }
        }

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


        const AST::VariablePtr retval = returnValue(alg);
        if (retval) {
            Arduino::Instruction loadRetval;
            loadRetval.type = Arduino::LOAD;
            findVariable(moduleId, id, retval, loadRetval.scope, loadRetval.arg);
            ret << loadRetval;

        }

        Arduino::Instruction retturn;
        retturn.type = Arduino::RET;
        ret << retturn;

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
            qCritical() << "\n***************\n";
            qCritical() << "Statement: " << types[st->type].c_str();
            qCritical() << "\n***************\n";
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
                    qCritical() << "\n***************\n";
                    qCritical() << "Loop type: " << loopTypes[st->loop.type].c_str();
                    qCritical() << "\n***************\n";
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
        const AST::ExpressionPtr rvalue = st->expressions[0];
        QList <Arduino::Instruction> rvalueInstructions = calculate(modId, algId, level, rvalue);
        result << rvalueInstructions;


        if (st->expressions.size() > 1) {
            const AST::ExpressionPtr lvalue = st->expressions[1];

            int diff = lvalue->operands.size() - lvalue->variable->dimension;

            if (diff > 0) {
                Arduino::Instruction load;
                findVariable(modId, algId, lvalue->variable, load.scope, load.arg);
                load.type = lvalue->variable->dimension > 0 ? Arduino::ARR : Arduino::VAR;
                for (int i = lvalue->variable->dimension - 1; i >= 0; i--) {
                    result << calculate(modId, algId, level, lvalue->operands[i]);
                }
                result << load;
            }

            if (diff == 1) {
                // Set character

                result << calculate(modId, algId, level,
                                    lvalue->operands[lvalue->operands.count() - 1]);
                Arduino::Instruction argsCount;
                argsCount.type = Arduino::VAR;
                argsCount.scope = Arduino::CONSTT;
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
                argsCount.arg = constantValue(Arduino::VT_int, 0, 4, QString(), QString());
                result << argsCount;
            }

            if (lvalue->kind == AST::ExprArrayElement) {
                for (int i = lvalue->variable->dimension - 1; i >= 0; i--) {
                    result << calculate(modId, algId, level, lvalue->operands[i]);
                }
            }

        }
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
            instr.arg = constId;
            result << instr;
        } else if (st->kind == AST::ExprVariable) {
            Arduino::Instruction instr;
            instr.type = Arduino::VAR;
            findVariable(modId, algId, st->variable, instr.scope, instr.arg);
            instr.varName = st->variable->name;
            result << instr;
        } else if (st->kind == AST::ExprArrayElement) {
            Arduino::Instruction instr;
            findVariable(modId, algId, st->variable, instr.scope, instr.arg);
            instr.type = Arduino::ARR;
            if (st->variable->dimension > 0) {
                for (int i = st->variable->dimension - 1; i >= 0; i--) {
                    result << innerCalculation(modId, algId, level, st->operands[i]);
                }
                instr.type = Arduino::VAR;
            }
            result << instr;
            int diff = st->operands.size() - st->variable->dimension;
            Arduino::Instruction argsCount;
            argsCount.type = Arduino::LOAD;
            argsCount.scope = Arduino::CONSTT;
            Arduino::Instruction specialFunction;
            specialFunction.type = Arduino::CALL;
            specialFunction.module = 0xff;
            if (diff == 1) {
                // Get char
                result << innerCalculation(modId, algId, level,
                                    st->operands[st->operands.count() - 1]);
                argsCount.arg = constantValue(Arduino::VT_int, 0, 2, QString(), QString());
                result << argsCount;
                specialFunction.arg = 0x04;
                result << specialFunction;
            } else if (diff == 2) {
                // Get slice
                result << innerCalculation(modId, algId, level,
                                    st->operands[st->operands.count() - 2]);
                result << innerCalculation(modId, algId, level,
                                    st->operands[st->operands.count() - 1]);
                argsCount.arg = constantValue(Arduino::VT_int, 0, 3, QString(), QString());
                result << argsCount;
                specialFunction.arg = 0x06;
                result << specialFunction;
            }
        } else if (st->kind == AST::ExprFunctionCall) {
            const AST::AlgorithmPtr alg = st->function;

            // Push calculable arguments to stack
            int argsCount = 0;
            for (int i = 0; i < st->operands.size(); i++) {
                AST::VariableAccessType t = alg->header.arguments[i]->accessType;
                bool arr = alg->header.arguments[i]->dimension > 0;
                if (t == AST::AccessArgumentOut || t == AST::AccessArgumentInOut) {
                    Arduino::Instruction func;
                    func.type = Arduino::FUNC;
                    findVariable(modId, algId, st->operands[i]->variable, func.scope, func.arg);
                    result << func;
                } else if (t == AST::AccessArgumentIn && !arr)
                    result << innerCalculation(modId, algId, level, st->operands[i]);
                else if (t == AST::AccessArgumentIn && arr) {
                    // load the whole array into stack
                    Arduino::Instruction load;
                    load.type = Arduino::ARR;
                    findVariable(modId, algId, st->operands[i]->variable, load.scope, load.arg);
                    result << load;
                }
                argsCount++;
            }
            Arduino::Instruction b;
            b.type = Arduino::CONST;
            b.scope = Arduino::CONSTT;
            b.arg = constantValue(Arduino::VT_int, 0, argsCount, QString(), QString());
            result << b;
            Arduino::Instruction instr;
            instr.type = Arduino::FUNC;
            findFunction(st->function, instr.module, instr.arg);
            result << instr;
        } else if (st->kind == AST::ExprSubexpression) {
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
            qCritical() << std::to_string(instr.arg).c_str();
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

void Generator::INIT(int modId, int algId, int level, const AST::StatementPtr  st, QList<Arduino::Instruction> & result)
{
    for (int i=0; i<st->variables.size(); i++) {
        const AST::VariablePtr  var = st->variables[i];
        if (var->dimension > 0 && var->bounds.size()>0) {
            for (int i=var->dimension-1; i>=0 ; i--) {
                result << calculate(modId, algId, level, var->bounds[i].second);
                result << calculate(modId, algId, level, var->bounds[i].first);
            }
            Arduino::Instruction bounds;
            bounds.type = Arduino::SETARR;
            findVariable(modId, algId, var, bounds.scope, bounds.arg);
            result << bounds;
        }
        else if (var->dimension > 0 && var->bounds.size()==0) {
            // TODO : Implement me after compiler support
        }
        Arduino::Instruction init;
        init.type = Arduino::INIT;
        findVariable(modId, algId, var, init.scope, init.arg);
        result << init;
        if (var->initialValue.isValid()) {
            Arduino::Instruction load;
            load.type = Arduino::LOAD;
            load.scope = Arduino::CONSTT;
            load.arg = constantValue(valueType(var->baseType), var->dimension, var->initialValue,
                                     var->baseType.actor ? var->baseType.actor->localizedModuleName(QLocale::Russian) : "",
                                     var->baseType.name
                                     );
            result << load;
        }
        else {
            // TODO constant values for dimension > 0
        }
    }
}

void Generator::CALL_SPECIAL(int modId, int algId, int level, const AST::StatementPtr  st, QList<Arduino::Instruction> & result)
{
    quint16 argsCount;


    if (st->type==AST::StOutput) {
        int varsCount = st->expressions.size() / 3;

        for (int i = 0; i<varsCount; ++i) {
            const AST::ExpressionPtr  expr = st->expressions[3*i];
            const AST::ExpressionPtr  format1 = st->expressions[3*i+1];
            const AST::ExpressionPtr  format2 = st->expressions[3*i+2];
            QList<Arduino::Instruction> instrs;
            instrs = calculate(modId, algId, level, expr);
            result << instrs;

            instrs = calculate(modId, algId, level, format1);
            result << instrs;

            instrs = calculate(modId, algId, level, format2);
            result << instrs;
        }

        if (st->expressions.size() % 3) {
            // File handle
            QList<Arduino::Instruction> instrs = calculate(modId, algId, level, st->expressions.last());
            result << instrs;
        }

        argsCount = st->expressions.size();
    }
    else if (st->type==AST::StInput) {
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

    Arduino::Instruction pushCount;
    pushCount.type = Arduino::LOAD;
    pushCount.scope = Arduino::CONSTT;
    pushCount.arg = constantValue(Arduino::VT_int, 0, argsCount, QString(), QString());
    result << pushCount;

    Arduino::Instruction call;
    call.type = Arduino::CALL;
    call.module = 0xFF;
    if (st->type==AST::StInput)
        call.arg = 0x0000;
    else if (st->type==AST::StOutput)
        call.arg = 0x0001;

    result << call;
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
        if (alg->impl.locals[i]->name == name)
            return alg->impl.locals[i];
    }
    return AST::VariablePtr();
}

void Generator::BREAK(int , int , int level,
                      const AST::StatementPtr  st,
                      QList<Arduino::Instruction> & result)
{
    Arduino::Instruction jump;
    jump.type = Arduino::JUMP;
    jump.type = Arduino::InstructionType(127);
    jump.registerr = level;

    result << jump;
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

            instr.type == Arduino::END_ST;
            result << instr;
            // Check condition result
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

        instrBuffer.type = Arduino::END_ST;
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
        workCondition.type = Arduino::END_ST;
        result << workCondition;

        QList <Arduino::Instruction> instructions = calculate(modId, algId, level, st->loop.fromValue);
        result << instructions;

        result << calculate(modId, algId, level, st->loop.toValue);
    }

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
}

} // namespace ArduinoCodeGenerator
