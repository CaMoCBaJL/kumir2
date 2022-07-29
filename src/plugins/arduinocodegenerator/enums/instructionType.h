//
// Created by anton on 7/17/22.
//

#ifndef KUMIR2_INSTRUCTIONTYPE_H
#define KUMIR2_INSTRUCTIONTYPE_H
namespace Arduino {

    enum InstructionType {
        ForLoop = 10000,
        WhileLoop = 10001,
        DCR = 10002,
        INC = 10003,
        VAR = 10004,
        ARR = 10005,
        FUNC = 10006,
        CONST = 10007,
        END_ST = 10008,
        END_VAR = 10009,
        END_ST_HEAD = 10010,
        INPUT = 10011,
        OUTPUT = 10012,
        STR_DELIM = 10013,
        BREAK = 10014,
        END_ARG = 10015,
        END_ARR = 10017,
        IF = 10018,
        ELSE = 10019,
        DO = 10020,
        START_SUB_EXPR = 10021,
        END_SUB_EXPR = 10022,
        CALL = 0x0A, // Call compiled function
        RET = 0x1B, // Return from function
        ERRORR,
        NOP,

        // Common operations -- no comments need

        SUM = 0xF1,
        SUB = 0xF2,
        MUL = 0xF3,
        DIV = 0xF4,
        POW = 0xF5,
        NEG = 0xF6,
        AND = 0xF7,
        OR = 0xF8,
        EQ = 0xF9,
        NEQ = 0xFA,
        LS = 0xFB,
        GT = 0xFC,
        LEQ = 0xFD,
        GEQ = 0xFE,
        ASG = 0xFF
    };
}
#endif //KUMIR2_INSTRUCTIONTYPE_H
