//
// Created by anton on 7/29/22.
//

#ifndef KUMIR2_ERRORGENERATOR_H
#define KUMIR2_ERRORGENERATOR_H
#include "../enums/enums.h"

namespace ArduinoCodeGenerator{
    using namespace Shared;

    void Generator::generateErrorInstruction(int, int, int, const AST::StatementPtr st, QList <Arduino::Instruction> &result) {
        const QString error = ErrorMessages::message("KumirAnalizer", QLocale::Russian, st->error);
        Arduino::Instruction e;
        e.type = Arduino::ERRORR;
        e.scope = Arduino::CONSTT;
        e.arg = calculateConstantValue(Arduino::VT_string, 0, error, QString(), QString());
        result << e;
    }
}
#endif //KUMIR2_ERRORGENERATOR_H
