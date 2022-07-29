//
// Created by anton on 7/29/22.
//

#ifndef KUMIR2_SWITCHGENERATOR_H
#define KUMIR2_SWITCHGENERATOR_H
#include "../enums/enums.h"
namespace ArduinoCodeGenerator {
    using namespace Shared;
    void Generator::generateChoiceInstruction(int modId, int algId, int level, const AST::StatementPtr st, QList<Arduino::Instruction> & result)
    {
        if (st->headerError.size()>0) {
            Arduino::Instruction garbage;
            garbage.type = Arduino::ERRORR;
            garbage.scope = Arduino::CONSTT;
            garbage.arg = calculateConstantValue(Arduino::VT_string, 0,
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
            err.arg = calculateConstantValue(Arduino::VT_string, 0, error, QString(), QString());
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
                err.arg = calculateConstantValue(Arduino::VT_string, 0, error, QString(), QString());
                result << err;
            }
            else {
                if (st->conditionals[i].condition) {
                    QList<Arduino::Instruction> condInstrs = calculateStatement(modId, algId, level, st->conditionals[i].condition);
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
}
#endif //KUMIR2_SWITCHGENERATOR_H
