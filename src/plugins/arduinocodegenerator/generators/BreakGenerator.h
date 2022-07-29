//
// Created by anton on 7/29/22.
//

#ifndef KUMIR2_BREAKGENERATOR_H
#define KUMIR2_BREAKGENERATOR_H
namespace ArduinoCodeGenerator{
#include "../enums/enums.h"
    void Generator::generateBreakInstruction(int , int , int , const AST::StatementPtr  st, QList<Arduino::Instruction> & result)
    {
        Arduino::Instruction a;
        a.type = Arduino::BREAK;
        a.arg = 0u;
        result << a;
    }
}
#endif //KUMIR2_BREAKGENERATOR_H
