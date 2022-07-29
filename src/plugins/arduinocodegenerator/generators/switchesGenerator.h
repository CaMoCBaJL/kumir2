//
// Created by anton on 7/29/22.
//

#ifndef KUMIR2_SWITCHGENERATOR_H
#define KUMIR2_SWITCHGENERATOR_H
#include "../enums/enums.h"
#include "kumir2-libs/dataformats/ast_statement.h"

namespace ArduinoCodeGenerator {
    using namespace Shared;

    void Generator::generateCase(int modId, int algId, int level, const AST::StatementPtr st,
                                 QList <Arduino::Instruction> &result, AST::ConditionSpec conditional) {
        Arduino::Instruction ifInstr;

        if (!conditional.conditionError.isEmpty()) {
            const QString error = ErrorMessages::message("KumirAnalizer", QLocale::Russian, conditional.conditionError);
            Arduino::Instruction err;
            err.type = Arduino::ERRORR;
            err.scope = Arduino::CONSTT;
            err.arg = calculateConstantValue(Arduino::VT_string, 0, error, QString(), QString());
            result << err;
        } else {
            if (conditional.condition) {
                QList <Arduino::Instruction> condInstrs = calculateStatement(modId, algId, level,
                                                                             conditional.condition);
                result << condInstrs;

                ifInstr.type = Arduino::END_ST_HEAD;
                result << ifInstr;
            }
            QList <Arduino::Instruction> instrs = calculateInstructions(modId, algId, level, conditional.body);
            result += instrs;

            ifInstr.type = Arduino::END_ST;
            result << ifInstr;
        }
    }

    void Generator::initCase(int modId, int algId, int level, const AST::StatementPtr st,
                             QList <Arduino::Instruction> &result, int counter) {
        Arduino::Instruction ifInstr;
        if (counter > 0) {
            ifInstr.type = Arduino::ELSE;
            result << ifInstr;
        }

        if (counter + 1 != st->conditionals.size()) {
            ifInstr.type = Arduino::IF;
            result << ifInstr;

            ifInstr.type = Arduino::START_SUB_EXPR;
            result << ifInstr;
        }
    }

    void Generator::checkForErrors(int modId, int algId, int level, const AST::StatementPtr st,
                                   QList <Arduino::Instruction> &result) {
        if (st->headerError.size() > 0) {
            Arduino::Instruction garbage;
            garbage.type = Arduino::ERRORR;
            garbage.scope = Arduino::CONSTT;
            garbage.arg = calculateConstantValue(Arduino::VT_string, 0,
                                                 ErrorMessages::message("KumirAnalizer", QLocale::Russian,
                                                                        st->headerError), QString(), QString()
            );
            result << garbage;
            return;
        }

        if (st->beginBlockError.size() > 0) {
            const QString error = ErrorMessages::message("KumirAnalizer", QLocale::Russian, st->beginBlockError);
            Arduino::Instruction err;
            err.type = Arduino::ERRORR;
            err.scope = Arduino::CONSTT;
            err.arg = calculateConstantValue(Arduino::VT_string, 0, error, QString(), QString());
            result << err;
            return;
        }
    }

    void Generator::generateChoiceInstruction(int modId, int algId, int level, const AST::StatementPtr st,
                                              QList <Arduino::Instruction> &result) {
        checkForErrors(modId, algId, level, st, result);

        for (int i = 0; i < st->conditionals.size(); i++) {
            initCase(modId, algId, level, st, result, i);

            generateCase(modId, algId, level, st, result, st->conditionals.at(i));
        }
    }
}
#endif //KUMIR2_SWITCHGENERATOR_H
