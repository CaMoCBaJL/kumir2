#ifndef ARDUINO_TABLEELEM_H
#define ARDUINO_TABLEELEM_H

#define DO_NOT_DECLARE_STATIC
#include <kumir2-libs/stdlib/kumirstdlib.hpp>

#include <string>
#include <sstream>
#include <vector>
#include "arduino_instruction.hpp"
#include "arduino_enums.hpp"
#include "CVariable.hpp"
#include <QDebug>
#include "generator.h"
#include <QtCore>

namespace Arduino {

using Kumir::String;
using Arduino::CVariable;
using Arduino::ElemType;
using Arduino::ValueType;
using Arduino::ValueKind;

struct TableElem {
    ElemType type; // Element type
    std::list<ValueType> vtype; // Value type
    uint8_t dimension; // 0 for regular, [1..3] for arrays
    ValueKind refvalue; // 1 for in-argument,
                     // 2 for in/out-argument,
                     // 3 for out-argument
                     // 0 for regular variable
    uint8_t module;  // module id
    uint16_t algId; // algorhitm id
    uint16_t id; // element id

    String name; // variable or function name
    std::string moduleAsciiName;
    String moduleLocalizedName; // external module name
    String fileName;
    String signature;
    std::string recordModuleAsciiName;
    String recordModuleLocalizedName;
    std::string recordClassAsciiName;
    String recordClassLocalizedName;
    CVariable initialValue; // constant value
    std::vector<Instruction> instructions; // for local defined function
    inline TableElem() {
        type = Arduino::EL_NONE;
        vtype.push_back(Arduino::VT_void);
        dimension = 0;
        refvalue = Arduino::VK_Plain;
        module = 0;
        algId = id = 0;
    }
};

static QList<QVariant> _constantElemValues;

inline std::string elemTypeToArduinoString(ElemType t)
{
    if (t==Arduino::EL_LOCAL)
        return "Arduino_local";
    else if (t==Arduino::EL_GLOBAL)
        return "Arduino_global";
    else if (t==Arduino::EL_CONST)
        return "Arduino_constant";
    else if (t==Arduino::EL_FUNCTION)
        return "func_name";
    else if (t==Arduino::EL_EXTERN)
        return "Arduino_extern";
    else if (t==Arduino::EL_INIT)
        return "void init_func";
    else if (t==Arduino::EL_MAIN)
        return "void main_func(){\n";
    else if (t==Arduino::EL_BELOWMAIN)
        return "Arduino_belowmain";
    else if (t==Arduino::EL_TESTING)
        return "void test_func";
    else  {
        return " elem type: " + std::to_string(t) + " ";
    }
}

inline std::string vtypeToString(ValueType instructionType)
    {
        switch(instructionType){
            case (Arduino::VT_int):
                return "int";
            case (Arduino::VT_float):
                return "float";
            case (Arduino::VT_char):
                return "char";
            case (Arduino::VT_string):
                return "string";
            case (Arduino::VT_bool):
                return "boolean";
            case (Arduino::VT_struct):
                return "struct{\n";
            default:
                return "";
        }
    }

inline std::string parseVType(const std::list<ValueType> & type, uint8_t dim)
    {
        std::string result;
        ValueType t = type.front();
        if (t == Arduino::VT_struct)
        {
            std::list<ValueType>::const_iterator it = type.begin();
            it ++;
            std::list<ValueType>::const_iterator itt;
            for ( ; it!=type.end(); ++it) {
                t = *it;
                result += vtypeToString(t);
                itt = it;
                itt++;
                if ( itt != type.end() ) {
                    result += ",";
                }
            }
            result += "}\n";
        }
        else
        if (result.length()>0) {
            for (uint8_t i=0; i<dim; i++) {
                result += "[]";
            }
        }
        else
            return vtypeToString(t);

        return result;
    }

inline void replaceAll(String &str, const String & from, const String & to)
{
    size_t fromSize = from.length();
    size_t toSize = to.size();
    size_t startPos = 0;
    while ((startPos = str.find(from, startPos))!=String::npos) {
        str.replace(startPos, fromSize, to);
        startPos += toSize;
    }
}

inline String screenString(String s)
{
    replaceAll(s, Kumir::Core::fromAscii("\\"), Kumir::Core::fromAscii("\\\\"));
    replaceAll(s, Kumir::Core::fromAscii("\n"), Kumir::Core::fromAscii("\\n"));
    replaceAll(s, Kumir::Core::fromAscii("\""), Kumir::Core::fromAscii("\\\""));
    replaceAll(s, Kumir::Core::fromAscii(" "), Kumir::Core::fromAscii("\\s"));
    replaceAll(s, Kumir::Core::fromAscii("\t"), Kumir::Core::fromAscii("\\t"));
    return s;
}

inline std::string externToTextStream(const TableElem & e) {
        std::ostringstream os;
        os.setf(std::ios::hex, std::ios::basefield);
        os.setf(std::ios::showbase);
        Kumir::EncodingError encodingError;
        os << "#include";
        os << "<" << Kumir::Coder::encode(Kumir::UTF8, screenString(e.moduleLocalizedName), encodingError) << ">;";
        os << "\"" << Kumir::Coder::encode(Kumir::UTF8, screenString(e.name), encodingError) << "\"";
        return os.str();
    }


inline std::string functionToTextStream(const TableElem & e, const AS_Helpers & helpers) {
    std::ostringstream os;
    os.setf(std::ios::hex, std::ios::basefield);
    os.setf(std::ios::showbase);
    if (e.type == Arduino::EL_MAIN){
        os << "void main(";
    } else {
        for (int16_t i = 0; i < e.instructions.size(); ++i){
            Instruction instr = e.instructions.at(i);
            if(instr.type == VAR) {
                if (instr.kind > 1){
                    os << parseValueType(instr.varType);
                    break;
                }
            }
        }
        os << std::string(e.name.begin(), e.name.end()) << '(';
    }

    os.unsetf(std::ios::basefield);
    os.unsetf(std::ios::showbase);
    for (size_t i = 0; i < e.instructions.size(); i++) {
        std::string parsedInstruction = instructionToString(e.instructions[i], helpers, e.module, e.algId,
                                                            _constantElemValues);
        os << parsedInstruction;
    }
    os << "\n";
    return os.str();
}

inline void tableElemToTextStream(std::ostream &ts, const TableElem &e, const AS_Helpers & helpers, const QList<QVariant> constants)
{
    _constantElemValues = constants;

    switch(e.type) {
        case Arduino::EL_EXTERN:
            ts << externToTextStream(e);
            break;
        default:
            ts << functionToTextStream(e, helpers);
            break;
    }
    ts << "\n";
}

} // namespace Arduino

#endif // ARDUINO_TABLEELEM_H
