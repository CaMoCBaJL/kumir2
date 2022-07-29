//
// Created by anton on 7/29/22.
//

#ifndef KUMIR2_ASSERTGENERATOR_H
#define KUMIR2_ASSERTGENERATOR_H
#include "../enums/enums.h"

namespace ArduinoCodeGenerator{
    void Generator::generateAssertInstruction(int modId, int algId, int level, const AST::StatementPtr  st, QList<Arduino::Instruction> & result) {
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
            exprInstrs = calculateStatement(modId, algId, level, st->expressions[i]);
            result << exprInstrs;
        }

        ifInstr.type = Arduino::END_SUB_EXPR;
        result << ifInstr;

        ifInstr.type = Arduino::END_ST_HEAD;
        result << ifInstr;
    }
}
#endif //KUMIR2_ASSERTGENERATOR_H
