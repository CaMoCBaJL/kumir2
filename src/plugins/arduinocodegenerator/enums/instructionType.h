//
// Created by anton on 7/17/22.
//

#ifndef KUMIR2_INSTRUCTIONTYPE_H
#define KUMIR2_INSTRUCTIONTYPE_H
namespace Arduino {

    enum InstructionType {
        ForLoop = 1,
        WhileLoop = 2,
        DO = 3,
        VAR = 4,
        ARR = 5,
        CONST = 6,
        FUNC = 7,
        INPUT = 8,
        OUTPUT = 9,
        CALL = 10,
        RET = 11,
        ERRORR = 12,
        BREAK = 13,
        IF = 14,
        ELSE = 15,

        STR_DELIM = 16,
        END_ARG = 17,
        END_ARR = 18,
        END_ST = 19,
        END_VAR = 20,
        END_ST_HEAD = 21,
        START_SUB_EXPR = 22,
        END_SUB_EXPR = 23,

        SUM = 24,
        SUB = 25,
        MUL = 26,
        DIV = 27,
        NEG = 28,
        AND = 29,
        OR = 30,
        EQ = 31,
        NEQ = 32,
        LS = 33,
        GT = 34,
        LEQ = 35,
        GEQ = 36,
        ASG = 37,
        DCR = 38,
        INC = 39,

        NOP = 40
    };
}
#endif //KUMIR2_INSTRUCTIONTYPE_H
