#ifndef ARDUINOGENERATOR_INSTRUCTION_H
#define ARDUINOGENERATOR_INSTRUCTION_H

#include <string>
#include <sstream>
#include <set>
#include <algorithm>
#define DO_NOT_DECLARE_STATIC
#include <kumir2-libs/stdlib/kumirstdlib.hpp>

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
    NOP         = 0x00,
    CALL        = 0x0A, // Call compiled function
    INIT        = 0x0C, // Initialize variable
    SETARR      = 0x0D, // Set array bounds
    STORE       = 0x0E, // Store value in variable
    STOREARR    = 0x0F, // Store value in array
    LOAD        = 0x10, // Get value from variable
    LOADARR     = 0x11, // Get value from array
    SETMON      = 0x12, // Set variable monitor
    UNSETMON    = 0x13, // Unset variable monitor
    JUMP        = 0x14, // Unconditional jump
    JNZ         = 0x15, // Conditional jump if non-zero value in specified register
    JZ          = 0x16, // Conditional jump if zero value in specified register
    POP         = 0x18, // Pop from stack to register
    PUSH        = 0x19, // Push to stack from register
    RET         = 0x1B, // Return from function
    PAUSE       = 0x1D, // Force pause
    ERRORR       = 0x1E, // Abort evaluation
    LINE        = 0x1F, // Emit line number
    REF         = 0x20, // Get reference to variable
    REFARR      = 0x21, // Get reference to array element
    SHOWREG     = 0x22, // Show register value at margin
    CLEARMARG   = 0x23, // Clear margin text from current line to specified
    SETREF      = 0x24, // Set reference value to variable
    HALT        = 0x26, // Terminate
    CTL         = 0x27, // Control VM behaviour
    INRANGE     = 0x28, // Pops 4 values ... a, b, c, x from stack Fand returns c>=0? a<x<=b : a<=x<b
    UPDARR      = 0x29, // Updates array bounds

    CSTORE      = 0x30, // Copy value from stack head and push it to cache
    CLOAD       = 0x31, // Pop value from cache to push it to main stack
    CDROPZ      = 0x32, // Drop cache value in case of zero value in specified register

    CACHEBEGIN  = 0x33, // Push begin marker into cache
    CACHEEND    = 0x34, // Clear cache until marker


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
        uint8_t module;
        uint8_t registerr;
    };
    uint16_t arg;
    QString varName;
};

    inline std::string parseInstructionData(Arduino::Instruction instruction, QList<QVariant> constants){
        if (instruction.varName.isNull()) {
            qCritical() << "\tinstruction data:";
            qCritical() << "arg: " << std::to_string(instruction.arg).c_str();
            qCritical() << "registerr: " << std::to_string(instruction.registerr).c_str();
            if (instruction.arg < constants.size()) {
                qCritical() << "constant at " << std::to_string(instruction.arg).c_str() << " "
                            << constants[instruction.arg].toString();
                return constants[instruction.arg].toString().toStdString();
            } else {
                qCritical() << "constant" << std::to_string(instruction.registerr).c_str() << " ";
                return std::to_string(instruction.registerr);
            }
        } else {
            return instruction.varName.toStdString();
        }


    }

    inline std::string parseOperationData(Arduino::Instruction instruction, QList<QVariant> constants) {
        QString result;

        switch (instruction.type) {
            case SUM:
                result.append('+');
                break;
            case SUB:
                result.append('-');
                break;
            case MUL:
                result.append('*');
                break;
            case DIV:
                result.append('/');
                break;
            case NEG:
                result.append('^');
                break;
            case AND:
                result.append("&&");
                break;
            case OR:
                result.append("||");
                break;
            case EQ:
                result.append("==");
                break;
            case NEQ:
                result.append("!=");
                break;
            case LS:
                result.append('<');
                break;
            case GT:
                result.append('>');
                break;
            case LEQ:
                result.append("<=");
                break;
            case GEQ:
                result.append(">=");
                break;
            case ASG:
                result.append("=");
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

static std::string typeToString(const Instruction &instruction, QList<QVariant> constants)
{
    InstructionType t = instruction.type;

    if (t==NOP) return ("nopArduino");
    else if (t==DCR) return parseOperationData(instruction, constants);
    else if (t==INC) return parseOperationData(instruction, constants);
    else if (t==END_ST) return ")\n";
    else if (t==END_VAR) return ";";
    else if (t==ForLoop) return ("for(");
    else if (t==WhileLoop) return ("while(");
    else if (t==VAR) return instruction.varName.toStdString();
    else if (t==CONST) return parseInstructionData(instruction, constants);
    else if (t==CALL) return (instruction.varName.toStdString() + "()");
    else if (t==SETARR) return ("setarrArduino");
    else if (t==STORE) return ("storeArduino");
    else if (t==STOREARR) return ("storearrArduino");
    else if (t==LOAD) return ("=");
    else if (t==LOADARR) return ("[]");
    else if (t==SETMON) return ("setmonArduino");
    else if (t==UNSETMON) return ("unsetmonArduino");
    else if (t==JUMP) return ("jump");
    else if (t==JNZ) return ("!=");
    else if (t==JZ) return ("==");
    else if (t==POP) return ("");
    else if (t==PUSH) return ("pushArduino");
    else if (t==RET) return ("\nreturn;\n}");
    else if (t==PAUSE) return ("\n}");
    else if (t==ERRORR) return ("errorArduino");
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
    else if (t==REF) return ("refArduino");
    else if (t==REFARR) return ("refarrArduino");
    else if (t==LINE) return ("lineArduino");
    else if (t==SHOWREG) return ("showregArduino");
    else if (t==CLEARMARG) return ("clearmargArduino");
    else if (t==SETREF) return ("setrefArduino");
    else if (t==PAUSE) return ("pauseArduino");
    else if (t==HALT) return ("haltArduino");
    else if (t==CTL) return ("ctlArduino");
    else if (t==INRANGE) return ("inrangeArduino");
    else if (t==UPDARR) return ("updarrArduino");
    else if (t==CLOAD) return ("cloadArduino");
    else if (t==CSTORE) return ("cstoreArduino");
    else if (t==CDROPZ) return ("cdropzArduino");
    else if (t==CACHEBEGIN) return ("cachebeginArduino");
    else if (t==CACHEEND) return ("cacheendArduino");
    else
    {
//        qCritical() << "instructionData";
//        qCritical() << std::to_string(instruction.type).c_str();
//        qCritical() << instruction.varName;
//        return " instruction: " + std::to_string(t) + " ";
        return "";
    }
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
    static std::set<InstructionType> VariableInstructions;
    VariableInstructions.insert(INIT);
    VariableInstructions.insert(SETARR);
    VariableInstructions.insert(STORE);
    VariableInstructions.insert(STOREARR);
    VariableInstructions.insert(LOAD);
    VariableInstructions.insert(LOADARR);
    VariableInstructions.insert(SETMON);
    VariableInstructions.insert(UNSETMON);
    VariableInstructions.insert(REF);
    VariableInstructions.insert(REFARR);
    VariableInstructions.insert(SETREF);
    VariableInstructions.insert(UPDARR);

    static std::set<InstructionType> ModuleNoInstructions;
    ModuleNoInstructions.insert(CALL);
    ModuleNoInstructions.insert(CTL);

    static std::set<InstructionType> RegisterNoInstructions;
    RegisterNoInstructions.insert(POP);
    RegisterNoInstructions.insert(PUSH);
    RegisterNoInstructions.insert(JZ);
    RegisterNoInstructions.insert(JNZ);
    RegisterNoInstructions.insert(SHOWREG);

    static std::set<InstructionType> HasValueInstructions;
    HasValueInstructions.insert(CALL);
    HasValueInstructions.insert(INIT);
    HasValueInstructions.insert(SETARR);
    HasValueInstructions.insert(STORE);
    HasValueInstructions.insert(STOREARR);
    HasValueInstructions.insert(LOAD);
    HasValueInstructions.insert(LOADARR);
    HasValueInstructions.insert(SETMON);
    HasValueInstructions.insert(UNSETMON);
    HasValueInstructions.insert(JUMP);
    HasValueInstructions.insert(JNZ);
    HasValueInstructions.insert(JZ);
    HasValueInstructions.insert(ERRORR);
    HasValueInstructions.insert(LINE);
    HasValueInstructions.insert(REF);
    HasValueInstructions.insert(REFARR);
    HasValueInstructions.insert(CLEARMARG);
    HasValueInstructions.insert(SETREF);
    HasValueInstructions.insert(HALT);
    HasValueInstructions.insert(PAUSE);
    HasValueInstructions.insert(CTL);
    HasValueInstructions.insert(UPDARR);

    std::stringstream result;
    result.setf(std::ios::hex,std::ios::basefield);
    result.setf(std::ios::showbase);
    InstructionType t = instr.type;
    result << typeToString(instr, constants);
    if (ModuleNoInstructions.count(t)) {
        result << " ";
    }
    std::string r = result.str();

    if (VariableInstructions.count(t)) {
//        VariableScope s = instr.scope;
//        AS_Key akey;
//        const AS_Values * vals = nullptr;
//        akey.first = 0;
//        if (s==LOCAL) {
//            akey.first = (moduleId << 16) | algId;
//            akey.second = instr.arg;
//            vals = &(helpers.locals);
//        }
//        else if (s==GLOBAL) {
//            akey.first = (moduleId << 16);
//            akey.second = instr.arg;
//            vals = &(helpers.globals);
//        }
//        else if (s==CONSTT) {
//            akey.first = 0;
//            akey.second = instr.arg;
//            vals = &(helpers.constants);
//        }
//        if (vals && vals->count(akey)) {
//            while (r.length()<25)
//                r.push_back(' ');
////            r += std::string("# ") + vals->at(akey);
//        }
    }
    else if (t==CALL) {
        AS_Key akey (instr.module << 16, instr.arg);
        const AS_Values * vals = &(helpers.algorithms);
        if (vals && vals->count(akey)) {
            while (r.length()<25)
                r.push_back(' ');
//            r += std::string("# ") + vals->at(akey);
        }
    }
//    r+= "\n";

    return r;
}

inline uint32_t toUint32(const Instruction &instr)
{
    static std::set<InstructionType> ModuleNoInstructions;
    ModuleNoInstructions.insert(CALL);
    ModuleNoInstructions.insert(CTL);

    static std::set<InstructionType> RegisterNoInstructions;
    RegisterNoInstructions.insert(POP);
    RegisterNoInstructions.insert(PUSH);
    RegisterNoInstructions.insert(JZ);
    RegisterNoInstructions.insert(JNZ);
    RegisterNoInstructions.insert(SHOWREG);

    uint32_t first = uint8_t(instr.type);
    first = first << 24;
    uint32_t second = 0u;
    if (ModuleNoInstructions.count(instr.type))
        second = uint8_t(instr.module);
    else if (RegisterNoInstructions.count(instr.type))
        second = uint8_t(instr.registerr);
    else
        second = uint8_t(instr.scope);
    if (instr.type == LINE) {
        second = uint8_t(instr.lineSpec);
    }
    second = second << 16;
    uint32_t last = instr.arg;
    last = last << 16; // Ensure first two bytes are 0x0000
    last = last >> 16;
    uint32_t result = first | second | last;
    return result;
}

inline Instruction fromUint32(uint32_t value)
{
    static std::set<InstructionType> ModuleNoInstructions;
    ModuleNoInstructions.insert(CALL);
    ModuleNoInstructions.insert(CTL);

    static std::set<InstructionType> RegisterNoInstructions;
    RegisterNoInstructions.insert(POP);
    RegisterNoInstructions.insert(PUSH);
    RegisterNoInstructions.insert(JZ);
    RegisterNoInstructions.insert(JNZ);
    RegisterNoInstructions.insert(SHOWREG);

    uint32_t first  = value & 0xFF000000;
    uint32_t second = value & 0x00FF0000;
    uint32_t last   = value & 0x0000FFFF;
    first = first >> 24;
    second = second >> 16;
    Instruction i;
    i.type = InstructionType(first);
    if (ModuleNoInstructions.count(i.type))
        i.module = uint8_t(second);
    else if (RegisterNoInstructions.count(i.type))
        i.registerr = uint8_t(second);
    else
        i.scope = VariableScope(second);
    if (i.type == LINE) {
        i.lineSpec = LineSpecification(second);
    }
    i.arg = uint16_t(last);
    return i;
}


} // end namespace Arduino

#endif // ARDUINOGENERATOR_INSTRUCTION_H
