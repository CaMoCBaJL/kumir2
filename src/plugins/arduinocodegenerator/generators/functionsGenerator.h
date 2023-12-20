//
// Created by anton on 7/29/22.
//

#ifndef KUMIR2_FUNCTIONSGENERATOR_H
#define KUMIR2_FUNCTIONSGENERATOR_H
#include "../enums/enums.h"
#include "../converters/ASTToArduinoConverter.h"

namespace ArduinoCodeGenerator {

    void Generator::generateTerminalWriteInstruction(int modId, int algId, int level, const AST::StatementPtr  st, QList<Arduino::Instruction> & result){
        Arduino::Instruction input;
        input.type = Arduino::INPUT;
        result << input;

        int varsCount = st->expressions.size();
        for (int i = varsCount-1; i>=0; i--) {
            const AST::ExpressionPtr  varExpr = st->expressions[i];
            Arduino::Instruction ref;
            if (varExpr->kind==AST::ExprConst) {
                ref.scope = Arduino::CONSTT;
                ref.arg = calculateConstantValue(parseValueType(varExpr->baseType), 0, varExpr->constant,
                                                 varExpr->baseType.actor ? varExpr->baseType.actor->localizedModuleName(QLocale::Russian) : "",
                                                 varExpr->baseType.name
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
                    QList<Arduino::Instruction> operandInstrs = calculateStatement(modId, algId, level, varExpr->operands[j]);
                    result << operandInstrs;
                }
                result << ref;

                ref.type = Arduino::END_ARR;
                result << ref;
            }
            else {
                QList<Arduino::Instruction> operandInstrs = calculateStatement(modId, algId, level, varExpr);
                result << operandInstrs;
            }
        }
    }

    void Generator::generateTerminalReadInstruction(int modId, int algId, int level, const AST::StatementPtr  st, QList<Arduino::Instruction> & result){
        int varsCount = st->expressions.size() / 3;
        Arduino::Instruction output;
        output.type = Arduino::OUTPUT;
        result << output;
        for (int i = 0; i<varsCount; ++i) {
            const AST::ExpressionPtr  expr = st->expressions[3*i];
            result << calculateStatement(modId, algId, level, expr);

            if (i + 1 < varsCount) {
                output.type = Arduino::SUM;
                result << output;
            }
        }

        if (st->expressions.size() % 3) {
            // File handle
            QList<Arduino::Instruction> instrs = calculateStatement(modId, algId, level, st->expressions.last());
            result << instrs;
        }
    }

    void Generator::generateTerminalIOInstruction(int modId, int algId, int level, const AST::StatementPtr  st, QList<Arduino::Instruction> & result)
    {
        if (st->type==AST::StOutput) {
            generateTerminalReadInstruction(modId, algId, level, st, result);
        }
        else if (st->type==AST::StInput) {
            generateTerminalWriteInstruction(modId, algId, level, st, result);
        }

        Arduino::Instruction endCall;
        endCall.type = Arduino::END_VAR;
        result << endCall;
    }

    const AST::VariablePtr Generator::generateReturnValue(const AST::AlgorithmPtr  alg)
    {
        const QString name = alg->header.name;
        for (int i=0; i<alg->impl.locals.size(); i++) {
            if (alg->impl.locals[i]->name == name) {
                return alg->impl.locals[i];
            }
        }
        return nullptr;
    }

    void Generator::initMainFunctionArguments(int moduleId, int algorhitmId, const AST::ModulePtr mod,
                               const AST::AlgorithmPtr alg, QList <Arduino::Instruction> & instrs){
        for (int i = 0; i < alg->header.arguments.size(); i++) {
            AST::VariablePtr var = alg->header.arguments[i];
            if (var->dimension > 0) {
                for (int j = var->dimension - 1; j >= 0; j--) {
                    QList <Arduino::Instruction> initBounds;
                    initBounds << calculateStatement(moduleId, algorhitmId, 0, var->bounds[j].second);
                    initBounds << calculateStatement(moduleId, algorhitmId, 0, var->bounds[j].first);
                    instrs << initBounds;
                }
                Arduino::Instruction bounds;
                bounds.type = Arduino::ARR;
                bounds.scope = Arduino::LOCAL;
                bounds.arg = quint16(i);
                instrs << bounds;
            }
        }
    }

    void Generator::addMainFunctionArguments(int moduleId, int algorhitmId, const AST::ModulePtr mod,
                              const AST::AlgorithmPtr alg, QList <Arduino::Instruction> & instrs){
        for (int i = 0; i < alg->header.arguments.size(); i++) {
            const AST::VariablePtr var = alg->header.arguments[i];
            Arduino::TableElem loc;
            loc.type = Arduino::EL_LOCAL;
            loc.module = moduleId;
            loc.algId = algorhitmId;
            loc.id = i;
            loc.name = var->name.toStdWString();
            loc.dimension = var->dimension;
            auto parsedType = parseValueType(var->baseType);
            loc.vtype = std::list<Arduino::ValueType>(parsedType.begin(), parsedType.end());
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
    }

    void Generator::addMainFunctionReturnValue(int moduleId, int algorhitmId, const AST::ModulePtr mod,
                                  const AST::AlgorithmPtr alg, QList <quint16> varsToOut){
        const AST::VariablePtr retval = generateReturnValue(alg);
        Arduino::TableElem loc;
        loc.type = Arduino::EL_LOCAL;
        loc.module = moduleId;
        loc.algId = algorhitmId;
        loc.id = 0;
        loc.name = tr("Function return value").toStdWString();
        loc.dimension = 0;
        auto parsedType = parseValueType(retval->baseType);
        loc.vtype = std::list<Arduino::ValueType>(parsedType.begin(), parsedType.end());
        loc.refvalue = Arduino::VK_Plain;
        byteCode_->d.push_back(loc);
        varsToOut << calculateConstantValue(Arduino::VT_int, 0, 0, QString(), QString());
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
            addMainFunctionReturnValue(moduleId, algorhitmId, mod, alg, varsToOut);
        }

        addMainFunctionArguments(moduleId, algorhitmId, mod, alg, instrs);

        initMainFunctionArguments(moduleId, algorhitmId, mod, alg, instrs);

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
        auto instructionsVector = instrs.toVector();
        func.instructions = std::vector<Arduino::Instruction>(instructionsVector.begin(), instructionsVector.end());
        byteCode_->d.push_back(func);

    }

    int Generator::findLastInstruction(QList<Arduino::Instruction> instructions, Arduino::InstructionType type){
        for (int i = instructions.size() - 1; i > -1; --i){
            if (instructions.at(i).type == type) {
                return i;
            }
        }

        return -1;
    }

    void Generator::checkForFunctionPartErrors(const AST::AlgorithmPtr alg,
                                    QList <Arduino::Instruction> & instructions, const QList<AST::LexemPtr> lexems){
        QString error = "";

        if (lexems.size() > 0) {
            for (int i = 0; i < lexems.size(); i++) {
                if (lexems[i]->error.size() > 0) {
                    error = ErrorMessages::message("KumirAnalizer", QLocale::Russian,
                                                   lexems[i]->error);
                    break;
                }
            }
        }

        if (error.length() > 0) {
            Arduino::Instruction err;
            err.type = Arduino::ERRORR;
            err.scope = Arduino::CONSTT;
            err.arg = calculateConstantValue(Arduino::VT_string, 0, error, QString(), QString());
            instructions << err;
        }
    }

    void Generator::declareFunctionHead(const AST::ModulePtr mod,
                               const AST::AlgorithmPtr alg, Arduino::TableElem & func, Arduino::ElemType type){
        func.type = type;
        func.module = mod->builtInID;
        func.algId = func.id;
        std::string algName = alg->header.name.toStdString();
        func.name = std::wstring(algName.begin(), algName.end());
        func.moduleLocalizedName = mod->header.name.toStdWString();
    }

    void Generator::addVariableArgument(QList <Arduino::Instruction> & argHandle, const AST::VariablePtr var,
                                        int counter, const AST::AlgorithmPtr alg){
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
        if (counter + 1 < alg->header.arguments.size()) {
            headerArg.type = Arduino::END_ARG;
            argHandle << headerArg;
        }
    }

    void Generator::addArrayArgument(QList <Arduino::Instruction> & argHandle, const AST::VariablePtr var,
                                     quint8 moduleId, int functionId){
        for (int i = var->dimension - 1; i >= 0; i--) {
            argHandle << calculateStatement(moduleId, functionId, 0, var->bounds[i].second);
            argHandle << calculateStatement(moduleId, functionId, 0, var->bounds[i].first);
        }
        Arduino::Instruction bounds;
        bounds.type = Arduino::VAR;
        findVariable(moduleId, functionId, var, bounds.scope, bounds.arg);
        argHandle << bounds;
    }

    void Generator::addFunctionArguments(const AST::ModulePtr mod,
                              const AST::AlgorithmPtr alg, QList <Arduino::Instruction> & argHandle, int functionId){
        for (int i = 0; i < alg->header.arguments.size(); i++) {
            const AST::VariablePtr var = alg->header.arguments[i];
            if (var->dimension > 0) {
                addArrayArgument(argHandle, var, mod->builtInID, functionId);
            } else {
                addVariableArgument(argHandle, var, i, alg);
            }
        }
    }

    void Generator::addProcedureReturnValue(const AST::VariablePtr retval, QList <Arduino::Instruction> & ret,
                                            QList <Arduino::Instruction> & argHandle){
        Arduino::Instruction delim;
        delim.type = Arduino::STR_DELIM;
        ret.push_front(delim);

        Arduino::Instruction loadRetval;
        loadRetval.type = Arduino::VAR;
        loadRetval.varType = Arduino::VT_None;
        loadRetval.varName = retval->name;
        ret << loadRetval;

        loadRetval.varName = QString();
        loadRetval.varType = parseVarType(retval);
        argHandle << loadRetval;
    }

    void Generator::addFunctionReturnValue(QList <Arduino::Instruction> & ret,
                                           QList <Arduino::Instruction> & argHandle,
                                           QList <Arduino::Instruction> & body){
        int lastArgIndex = findLastInstruction(argHandle, Arduino::VAR);
        if (lastArgIndex > -1 && argHandle.at(lastArgIndex).kind > 1) {
            Arduino::Instruction funcResult;
            funcResult.type = Arduino::VAR;
            funcResult.varName = argHandle.at(lastArgIndex).varName;
            ret << funcResult;
            Arduino::Instruction retArg = argHandle.at(lastArgIndex);
            retArg.varName = QString();

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

    void Generator::generateFunctionReturnValue(QList <Arduino::Instruction> & body, QList <Arduino::Instruction> & ret,
                                                QList <Arduino::Instruction> & argHandle, const AST::AlgorithmPtr alg,
                                                QList <Arduino::Instruction> & pre){
        const AST::VariablePtr retval = generateReturnValue(alg);
        if (retval != nullptr) {
            addProcedureReturnValue(retval, ret, argHandle);
        } else {
            addFunctionReturnValue(ret, argHandle, body);
        }

        int returnValueIndex;

        for (int i = argHandle.size() - 1; i > 0; --i){
            if (argHandle.at(i).type == Arduino::VAR){
                if (argHandle.at(i).varName == QString()) {
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

        Arduino::Instruction retturn;
        retturn.type = Arduino::END_VAR;
        ret << retturn;
        retturn.type = Arduino::END_ST;
        ret << retturn;
    }

    void Generator::addFunction(int id, int moduleId, Arduino::ElemType type, const AST::ModulePtr mod,
                                const AST::AlgorithmPtr alg) {
        Arduino::TableElem func;

        QList <Arduino::Instruction> argHandle;

        checkForFunctionPartErrors(alg, argHandle, alg->impl.headerLexems);
        checkForFunctionPartErrors(alg, argHandle, alg->impl.beginLexems);

        declareFunctionHead(mod, alg, func, type);

        addFunctionArguments(mod, alg, argHandle, id);

        Arduino::Instruction endFuncDeclaration;
        endFuncDeclaration.type = Arduino::END_ST_HEAD;
        argHandle << endFuncDeclaration;

        QList <Arduino::Instruction> pre = calculateInstructions(moduleId, id, 0, alg->impl.pre);
        QList <Arduino::Instruction> body = calculateInstructions(moduleId, id, 0, alg->impl.body);
        QList <Arduino::Instruction> post = calculateInstructions(moduleId, id, 0, alg->impl.post);

        QList <Arduino::Instruction> ret;

        checkForFunctionPartErrors(alg, ret, alg->impl.endLexems);

        Arduino::Instruction retturn;
        retturn.type = Arduino::RET;
        ret << retturn;

        generateFunctionReturnValue(body, ret, argHandle, alg, pre);

        QList <Arduino::Instruction> instrs = argHandle + pre + body + post + ret;
        auto instructionsVector = instrs.toVector();
        func.instructions = std::vector<Arduino::Instruction>(instructionsVector.begin(), instructionsVector.end());
        byteCode_->d.push_back(func);
    }
}
#endif //KUMIR2_FUNCTIONSGENERATOR_H
