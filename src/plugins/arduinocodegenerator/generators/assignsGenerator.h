//
// Created by anton on 7/29/22.
//

#ifndef KUMIR2_ASSIGNGENERATOR_H
#define KUMIR2_ASSIGNGENERATOR_H
#include "../converters/ASTToArduinoConverter.h"

namespace ArduinoCodeGenerator{
    void Generator::generateAssignInstruction(int modId, int algId, int level, const AST::StatementPtr st,
                                              QList <Arduino::Instruction> &result) {
        if (st->expressions.size() > 1) {
            const AST::ExpressionPtr lvalue = st->expressions[1];
            int diff = lvalue->operands.size() - lvalue->variable->dimension;
            if (diff == 0) {
                Arduino::Instruction lVar;
                findVariable(modId, algId, lvalue->variable, lVar.scope, lVar.arg);
                lVar.type = Arduino::VAR;

                for (int i = lvalue->variable->dimension - 1; i >= 0; i--) {
                    result << calculateStatement(modId, algId, level, lvalue->operands[i]);
                }

                lVar.varName = lvalue->variable->name;
                lVar.varType = Arduino::VT_None;

                result << lVar;
            }

            if (diff == 1) {
                // Set character

                result << calculateStatement(modId, algId, level,
                                    lvalue->operands[lvalue->operands.count() - 1]);
                Arduino::Instruction argsCount;
                argsCount.type = Arduino::VAR;
                argsCount.scope = Arduino::CONSTT;
                argsCount.varType = parseVarType(lvalue->variable);
                argsCount.arg = calculateConstantValue(Arduino::VT_int, 0, 3, QString(), QString());
                result << argsCount;
            }

            if (diff == 2) {
                // Set slice

                result << calculateStatement(modId, algId, level,
                                    lvalue->operands[lvalue->operands.count() - 2]);
                result << calculateStatement(modId, algId, level,
                                    lvalue->operands[lvalue->operands.count() - 1]);
                Arduino::Instruction argsCount;
                argsCount.type = Arduino::ARR;
                argsCount.scope = Arduino::CONSTT;
                argsCount.varType = parseVarType(lvalue->variable);
                argsCount.arg = calculateConstantValue(Arduino::VT_int, 0, 4, QString(), QString());
                result << argsCount;
            }

            if (lvalue->kind == AST::ExprArrayElement) {
                for (int i = lvalue->variable->dimension - 1; i >= 0; i--) {
                    result << calculateStatement(modId, algId, level, lvalue->operands[i]);
                }
            }

            Arduino::Instruction asg;
            asg.type = Arduino::ASG;
            result << asg;
        }

        const AST::ExpressionPtr rvalue = st->expressions[0];
        QList <Arduino::Instruction> rvalueInstructions = calculateStatement(modId, algId, level, rvalue);
        result << rvalueInstructions;

        Arduino::Instruction endAsg;
        if (rvalue->kind == AST::ExprFunctionCall) {
            endAsg.type = Arduino::END_SUB_EXPR;
            result << endAsg;
        }
        endAsg.type = Arduino::END_VAR;
        result << endAsg;
    }
}
#endif //KUMIR2_ASSIGNGENERATOR_H
