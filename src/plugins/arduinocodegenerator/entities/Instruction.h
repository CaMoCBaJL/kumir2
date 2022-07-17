//
// Created by anton on 7/17/22.
//
#ifndef KUMIR2_INSTRUCTION_H
#define KUMIR2_INSTRUCTION_H
#include "../enums/enums.h"
#include <stdint.h>
#include "QString"

namespace Arduino{
    struct Instruction {
        InstructionType type;
        union {
            LineSpecification lineSpec;
            VariableScope scope;
            ValueKind kind;
            uint8_t module;
            uint8_t registerr;
        };
        uint16_t arg;
        QString varName;
        Arduino::ValueType varType;
    };
}
#endif //KUMIR2_INSTRUCTION_H
