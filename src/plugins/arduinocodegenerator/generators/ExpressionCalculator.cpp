//
// Created by anton on 7/18/22.
//

#include "ExpressionCalculator.hpp"
#include "../enums/enums.h"
#include <QList>
#include "kumir2-libs/dataformats/ast_expression.h"
#include "kumir2-libs/dataformats/ast_statement.h"
#include "kumir2-libs/dataformats/ast_variable.h"
#include "kumir2-libs/dataformats/ast_algorhitm.h"

namespace ArduinoCodeGenerator{
    ExpressionCalculator::ExpressionCalculator() {

    };

    Arduino::InstructionType ExpressionCalculator::parseInstructionType(AST::ExpressionPtr st) {
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

    QList<Arduino::Instruction> ExpressionCalculator::innerCalculation(int modId, int algId, int level, const AST::ExpressionPtr st){
        QList <Arduino::Instruction> result;
        if (st->kind == AST::ExprConst) {
            int constId = constantValue(valueType(st->baseType), st->dimension, st->constant,
                                        st->baseType.actor ? st->baseType.actor->localizedModuleName(QLocale::Russian)
                                                           : "",
                                        st->baseType.name , this->constants_
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
                    result << innerCalculation(modId, algId, level, st->operands[i]);
                }
            }
            result << instr;
            int diff = st->operands.size() - st->variable->dimension;
            Arduino::Instruction argsCount;
            argsCount.type = Arduino::ARR;
            argsCount.scope = Arduino::CONSTT;
            if (diff == 1) {
                // Get char
                result << innerCalculation(modId, algId, level,
                                           st->operands[st->operands.count() - 1]);
                argsCount.arg = constantValue(Arduino::VT_int, 0, 2, QString(), QString(), this->constants_);
                result << argsCount;
            } else if (diff == 2) {
                // Get slice
                result << innerCalculation(modId, algId, level,
                                           st->operands[st->operands.count() - 2]);
                result << innerCalculation(modId, algId, level,
                                           st->operands[st->operands.count() - 1]);
                argsCount.arg = constantValue(Arduino::VT_int, 0, 3, QString(), QString(), this->constants_);
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
                    result << calculate(modId, algId, level, st->operands.at(i));
                    continue;
                }

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
                Arduino::Instruction instr;
                instr.type = parseInstructionType(st);
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

    Arduino::Instruction ExpressionCalculator::parseConstOrVarExpr(AST::ExpressionPtr expr){
        Arduino::Instruction instr;

        if (expr->variable){
            instr.type = Arduino::VAR;
            instr.varName = expr->variable->name;
            instr.varType = Arduino::VT_None;
        } else{
            instr.type = Arduino::CONST;
            instr.arg = constantValue(Arduino::VT_string, 0, expr->constant, QString(), QString(), this->constants_);
        }

        return instr;
    }

    QList<Arduino::Instruction> ExpressionCalculator::getOperands(AST::ExpressionPtr expr){
        QList<Arduino::Instruction> result;
        for (uint16_t i = 0; i < expr->operands.size(); i++){
            AST::ExpressionPtr e = expr->operands.at(i);

            bool hasChildren;

            for (int j = 0; j < expr->operands.size(); ++j){
                if (expr->operands.at(i)->operands.size() != 0){
                    hasChildren = true;
                }
            }

            if (i == 1 && hasChildren){
                Arduino::Instruction instr;
                instr.type = parseInstructionType(expr);
                result << instr;
            }

            if (expr->operands.at(i)->operands.size() == 0){
                result << parseConstOrVarExpr(expr->operands.at(i));
                continue;
            }
            else{
                Arduino::Instruction brace;
                if (e->expressionIsClosed){
                    brace.type = Arduino::START_SUB_EXPR;
                    result << brace;
                }
                result << getOperands(expr->operands.at(i));

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

    QList <Arduino::Instruction> ExpressionCalculator::calculate(int modId, int algId, int level, const AST::ExpressionPtr st) {
        QList<Arduino::Instruction> result = innerCalculation(modId, algId, level, st);

        if (st->kind == AST::ExprSubexpression){
            result = getOperands(st);
        }

        return result;
    }
}