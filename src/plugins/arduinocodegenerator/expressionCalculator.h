//
// Created by anton on 7/29/22.
//

#ifndef KUMIR2_EXPRESSIONCALCULATOR_H
#define KUMIR2_EXPRESSIONCALCULATOR_H
namespace ArduinoCodeGenerator{
    QList<Arduino::Instruction> Generator::parseStatement(int modId, int algId, int level, const AST::ExpressionPtr st){
        QList <Arduino::Instruction> result;
        if (st->kind == AST::ExprConst) {
            int constId = calculateConstantValue(parseValueType(st->baseType), st->dimension, st->constant,
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
            result << instr;
        } else if (st->kind == AST::ExprArrayElement) {
            Arduino::Instruction instr;
            findVariable(modId, algId, st->variable, instr.scope, instr.arg);
            instr.type = Arduino::ARR;
            instr.varName = st->variable->name;
            result << instr;
            if (st->variable->dimension > 0) {
                for (int i = st->variable->dimension - 1; i >= 0; i--) {
                    result << parseStatement(modId, algId, level, st->operands[i]);
                }
            }
            result << instr;
            int diff = st->operands.size() - st->variable->dimension;
            Arduino::Instruction argsCount;
            argsCount.type = Arduino::ARR;
            argsCount.scope = Arduino::CONSTT;
            if (diff == 1) {
                // Get char
                result << parseStatement(modId, algId, level,
                                         st->operands[st->operands.count() - 1]);
                argsCount.arg = calculateConstantValue(Arduino::VT_int, 0, 2, QString(), QString());
                result << argsCount;
            } else if (diff == 2) {
                // Get slice
                result << parseStatement(modId, algId, level,
                                         st->operands[st->operands.count() - 2]);
                result << parseStatement(modId, algId, level,
                                         st->operands[st->operands.count() - 1]);
                argsCount.arg = calculateConstantValue(Arduino::VT_int, 0, 3, QString(), QString());
                result << argsCount;
            }
        } else if (st->kind == AST::ExprFunctionCall) {
            const AST::AlgorithmPtr alg = st->function;

            for (int i = 0; i < st->operands.size(); i++) {
                AST::VariablePtr argument = st->operands.at(i)->variable;
                if (argument && !argument->name.isNull()) {
                    Arduino::Instruction instr;
                    instr.type = Arduino::VAR;
                    instr.varName = argument->name;
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
                if (i > 0){
                    Arduino::Instruction argDelim;
                    argDelim.type = Arduino::END_ARG;
                    result << argDelim;
                }

                AST::VariableAccessType t = alg->header.arguments[i]->accessType;
                bool arr = alg->header.arguments[i]->dimension > 0;

                if (st->operands.at(i)->kind == AST::ExprSubexpression){
                    result << calculateStatement(modId, algId, level, st->operands.at(i));
                    continue;
                }

                if (t == AST::AccessArgumentIn && !arr) {
                    result << parseStatement(modId, algId, level, st->operands[i]);
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
                Arduino::Instruction instr;
                instr.type = parseInstructionType(st);
                if (operand->operatorr > 0 && i - 2 > 0 &&
                    (operands.at(i - 2)->operatorr == AST::OpNone && operands.at(i - 1)->operatorr == AST::OpNone)) {
                    operands.replace(i - 1, operand);
                }
            }
            for (int i = 0; i < operands.size(); i++) {
                AST::ExpressionPtr operand = operands.at(i);
                instrs.append(parseStatement(modId, algId, level, operand));
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

    QList<Arduino::Instruction> Generator::additionalSubExpressionCalculations(AST::ExpressionPtr expr){
        QList<Arduino::Instruction> result;
        for (uint16_t i = 0; i < expr->operands.size(); i++){
            AST::ExpressionPtr e = expr->operands.at(i);

            bool hasChildren;

            for (int j = 0; j < expr->operands.size(); ++j){
                if (expr->operands.at(i)->operands.size() != 0){
                    hasChildren = true;
                    break;
                }
            }

            if (i == 1 && hasChildren){
                Arduino::Instruction instr;
                if (expr->operatorr != AST::OpOr && expr->operatorr != AST::OpAnd) {
                    instr.type = parseInstructionType(expr);
                    result << instr;
                }
            }

            if (expr->operands.at(i)->operands.size() == 0) {
                result << parseConstOrVarExpr(expr->operands.at(i));
                continue;
            }
            else{
                Arduino::Instruction brace;
                if (e->expressionIsClosed){
                    brace.type = Arduino::START_SUB_EXPR;
                    result << brace;
                }
                result << additionalSubExpressionCalculations(expr->operands.at(i));

                if (e->expressionIsClosed){
                    brace.type = Arduino::END_SUB_EXPR;
                    result << brace;
                }
                if (i + 1 == expr->operands.size()){
                    break;
                }

                if (expr->operatorr == AST::OpOr || expr->operatorr == AST::OpAnd) {
                    Arduino::Instruction instr;
                    instr.type = parseInstructionType(expr);
                    result << instr;
                }
            }
        }

        return result;
    }

    QList <Arduino::Instruction> Generator::calculateStatement(int modId, int algId, int level, const AST::ExpressionPtr st) {
        QList<Arduino::Instruction> result = parseStatement(modId, algId, level, st);

        if (st->kind == AST::ExprSubexpression){
            result = additionalSubExpressionCalculations(st);
        }

        return result;
    }
}
#endif //KUMIR2_EXPRESSIONCALCULATOR_H
