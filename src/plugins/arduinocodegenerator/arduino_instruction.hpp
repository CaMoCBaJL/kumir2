#ifndef ARDUINOGENERATOR_INSTRUCTION_H
#define ARDUINOGENERATOR_INSTRUCTION_H

#include <string>
#include <sstream>
#include <set>
#include <algorithm>
#define DO_NOT_DECLARE_STATIC
#include <kumir2-libs/stdlib/kumirstdlib.hpp>
#include "arduino_enums.hpp"

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
    CALL        = 0x0A, // Call compiled function
    RET         = 0x1B, // Return from function
    ERRORR,
    NOP,

    // Common operations -- no comments need

    SUM         = 0xF1,
    SUB         = 0xF2,
    MUL         = 0xF3,
    DIV         = 0xF4,
    POW         = 0xF5,
    NEG         = 0xF6,
    AND         = 0xF7,
    OR          = 0xF8,
    EQ          = 0xF9,
    NEQ         = 0xFA,
    LS          = 0xFB,
    GT          = 0xFC,
    LEQ         = 0xFD,
    GEQ         = 0xFE,
    ASG = 0xFF
};

enum VariableScope {
    UNDEF = 0x00, // Undefined if not need
    CONSTT = 0x01, // Value from constants table
    LOCAL = 0x02, // Value from locals table
    GLOBAL= 0x03  // Value from globals table
};

enum LineSpecification {
    LINE_NUMBER = 0x00,
    COLUMN_START_AND_END = 0x80
};

/* Instruction has optimal (aka serialized) size of 32 bit:
  - first byte is Instruction Type
  - second byte is Module Number (for CALL),
    register number (for POP, PUSH, JZ and JNZ)
    variable scope (for variable-access instructions) or
    undefined (e.g. value is 0x00)
  - last two bytes is instruction argument value
    (or 0x0000 if not defined)
*/

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

static std::string parseValueType(Arduino::ValueType type){
    switch (type){
        case VT_void: return "void ";
        case VT_int: return "int ";
        case VT_float: return "float ";
        case VT_char: return "char ";
        case VT_bool: return "bool ";
        case VT_string: return "std::string ";
        case VT_struct: return "struct ";
        case VT_None: return "";
        default: return "";
    }
}

    inline std::string parseInstructionData(Arduino::Instruction instruction, QList<QVariant> constants) {
        if (instruction.type == ARR) {
            return parseValueType(instruction.varType) + instruction.varName.toStdString() + "[";
        }
        if (instruction.varName.isNull()) {
            std::string result;
            if (instruction.type == VAR){
                return result;
            }

            if (instruction.arg < constants.size()) {
                result = constants[instruction.arg].toString().toStdString();
                if (constants[instruction.arg].type() == QMetaType::QString) {
                    result = "\"" + result + "\"";
                } else {
                    result = result;
                }

            } else {
                result = std::to_string(instruction.registerr);
            }
            return result;
        } else {
            return parseValueType(instruction.varType) + instruction.varName.toStdString();
        }
    }

    inline std::string parseOperationData(Arduino::Instruction instruction, QList<QVariant> constants) {
        QString result;

        switch (instruction.type) {
            case SUM:
                result.append(" + ");
                break;
            case SUB:
                result.append(" - ");
                break;
            case MUL:
                result.append(" * ");
                break;
            case DIV:
                result.append(" / ");
                break;
            case NEG:
                result.append(" ! ");
                break;
            case AND:
                result.append(" && ");
                break;
            case OR:
                result.append(" || ");
                break;
            case EQ:
                result.append(" == ");
                break;
            case NEQ:
                result.append(" != ");
                break;
            case LS:
                result.append(" < ");
                break;
            case GT:
                result.append(" > ");
                break;
            case LEQ:
                result.append(" <= ");
                break;
            case GEQ:
                result.append(" >= ");
                break;
            case ASG:
                result.append(" = ");
                break;
            case DCR:
                result.append("--");
                break;
            case INC:
                result.append("++");
                break;
        }
        
        return result.toStdString();
    }

inline std::string parseFunctionInstruction(const Instruction instruction){
    std::string result;
    if (instruction.varType > 0){
        result = parseValueType(instruction.varType);
    }

    result += instruction.varName.toStdString() + "(";
    return result;
}

static std::string typeToString(const Instruction &instruction, QList<QVariant> constants)
{
    InstructionType t = instruction.type;

    if (t==OUTPUT) return "cout << ";
    else if (t==INPUT) return "cin >> ";
    else if (t==BREAK) return "break;";
    else if (t==STR_DELIM) return "\n";
    else if (t==END_ARG) return ", ";
    else if (t==END_ARR) return "]";
    else if (t==START_SUB_EXPR) return "(";
    else if (t==END_SUB_EXPR) return ")";
    else if (t==FUNC) return parseFunctionInstruction(instruction);
    else if (t==DCR) return parseOperationData(instruction, constants);
    else if (t==INC) return parseOperationData(instruction, constants);
    else if (t==END_ST) return "\n}";
    else if (t==END_VAR) return ";";
    else if (t==DO) return "do{\n";
    else if (t==END_ST_HEAD) return ")\n{\n";
    else if (t==ForLoop) return ("for ");
    else if (t==WhileLoop) return ("while ");
    else if (t==VAR) return parseInstructionData(instruction, constants);
    else if (t==CONST) return parseInstructionData(instruction, constants);
    else if (t==CALL) return instruction.varName.toStdString() + "()";
    else if (t==ARR) return parseInstructionData(instruction, constants);
    else if (t==RET) return ("return ");
    else if (t==IF) return "if ";
    else if (t==ELSE) return "else ";
    else if (t==SUM) return parseOperationData(instruction, constants);
    else if (t==SUB) return parseOperationData(instruction, constants);
    else if (t==MUL) return parseOperationData(instruction, constants);
    else if (t==DIV) return parseOperationData(instruction, constants);
    else if (t==POW) return parseOperationData(instruction, constants);
    else if (t==NEG) return parseOperationData(instruction, constants);
    else if (t==AND) return parseOperationData(instruction, constants);
    else if (t==OR) return parseOperationData(instruction, constants);
    else if (t==EQ) return parseOperationData(instruction, constants);
    else if (t==NEQ) return parseOperationData(instruction, constants);
    else if (t==LS) return parseOperationData(instruction, constants);
    else if (t==GT) return parseOperationData(instruction, constants);
    else if (t==LEQ) return parseOperationData(instruction, constants);
    else if (t==GEQ) return parseOperationData(instruction, constants);
    else if (t==ASG) return parseOperationData(instruction, constants);
    else if (t==ERRORR) return "\n\nERROR! \n\n" + parseInstructionData(instruction, constants);
    else return "";
}

typedef std::pair<uint32_t, uint16_t> AS_Key;
    // module | algorithm, id
typedef std::map<AS_Key, std::string> AS_Values;
struct AS_Helpers {
    AS_Values globals;
    AS_Values locals;
    AS_Values constants;
    AS_Values algorithms;
};

inline std::string instructionToString(const Instruction &instr, const AS_Helpers & helpers, uint8_t moduleId, uint16_t algId, QList<QVariant> constants)
{
    static std::set<InstructionType> ModuleNoInstructions;
    ModuleNoInstructions.insert(CALL);

    static std::set<InstructionType> HasValueInstructions;
    HasValueInstructions.insert(CALL);

    std::stringstream result;
    result.setf(std::ios::hex,std::ios::basefield);
    result.setf(std::ios::showbase);
    InstructionType t = instr.type;
    result << typeToString(instr, constants);
    if (ModuleNoInstructions.count(t)) {
        result << " ";
    }
    std::string r = result.str();

    if (t==CALL) {
        AS_Key akey (instr.module << 16, instr.arg);
        const AS_Values * vals = &(helpers.algorithms);
        if (vals && vals->count(akey)) {
            while (r.length()<25)
                r.push_back(' ');
        }
    }

    return r;
}

} // end namespace Arduino

#endif // ARDUINOGENERATOR_INSTRUCTION_H
