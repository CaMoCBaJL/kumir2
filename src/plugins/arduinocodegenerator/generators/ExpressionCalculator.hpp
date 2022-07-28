//
// Created by anton on 7/18/22.
//

#ifndef KUMIR2_EXPRESSIONCALCULATOR_HPP
#define KUMIR2_EXPRESSIONCALCULATOR_HPP
#include "kumir2-libs/dataformats/ast_expression.h"
#include "../entities/Instruction.h"
#include "../enums/InstructionType.h"
#include <QList>
namespace ArduinoCodeGenerator {
    class ExpressionCalculator {
    public:
        ExpressionCalculator();
        QList <Arduino::Instruction> calculate(int modId, int algId, int level, const AST::ExpressionPtr st);
    private:
        QList <Arduino::Instruction> innerCalculation(int modId, int algId, int level, const AST::ExpressionPtr st);
        Arduino::InstructionType parseInstructionType(AST::ExpressionPtr st);
        Arduino::Instruction parseConstOrVarExpr(AST::ExpressionPtr expr);
        QList<Arduino::Instruction> getOperands(AST::ExpressionPtr expr);
    };
}

#endif //KUMIR2_EXPRESSIONCALCULATOR_HPP
