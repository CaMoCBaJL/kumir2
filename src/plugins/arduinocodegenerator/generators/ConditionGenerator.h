//
// Created by anton on 7/29/22.
//

#ifndef KUMIR2_CONDITIONGENERATOR_H
#define KUMIR2_CONDITIONGENERATOR_H
#include "../enums/enums.h"
namespace ArduinoCodeGenerator{
    using namespace Shared;

    void Generator::generateConditionInstruction(int modId, int algId, int level, const AST::StatementPtr  st, QList<Arduino::Instruction> &result)
    {
        Arduino::Instruction ifInstr;
        ifInstr.type = Arduino::IF;
        result << ifInstr;

        ifInstr.type = Arduino::START_SUB_EXPR;
        result << ifInstr;

        if (st->conditionals[0].condition) {
            QList<Arduino::Instruction> conditionInstructions = calculateStatement(modId, algId, level, st->conditionals[0].condition);
            result << conditionInstructions;
            ifInstr.type = Arduino::END_ST_HEAD;
            result << ifInstr;
        }

        Arduino::Instruction error;
        if (st->conditionals[0].conditionError.size()>0) {
            const QString msg = ErrorMessages::message("KumirAnalizer", QLocale::Russian, st->conditionals[0].conditionError);
            error.type = Arduino::ERRORR;
            error.scope = Arduino::CONSTT;
            error.arg = calculateConstantValue(Arduino::VT_string, 0, msg, QString(), QString());
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
                error.arg = calculateConstantValue(Arduino::VT_string, 0, msg, QString(), QString());
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
            error.arg = calculateConstantValue(Arduino::VT_string, 0, msg, QString(), QString());
            result << error;
        }

    }
}
#endif //KUMIR2_CONDITIONGENERATOR_H
