#ifndef ARDUINO_TABLEELEM_H
#define ARDUINO_TABLEELEM_H

#define DO_NOT_DECLARE_STATIC
#include <kumir2-libs/stdlib/kumirstdlib.hpp>

#include <string>
#include <sstream>
#include <vector>
#include "entities/arduinoVariable.h"
#include <QDebug>
#include "generator.h"
#include <QtCore>
#include "entities/arduinoTableElem.h"
#include "enums/enums.h"
#include "entities/arduinoInstruction.h"
#include "converters/arduinoToStringConverter.h"
#include "entities/arduinoHelpers.h"
namespace Arduino {

static QList<QVariant> _constantElemValues;

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
