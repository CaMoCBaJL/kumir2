#ifndef ARDUINOGENERATOR_INSTRUCTION_H
#define ARDUINOGENERATOR_INSTRUCTION_H

#include <string>
#include <sstream>
#include <set>
#include <algorithm>
#define DO_NOT_DECLARE_STATIC
#include <kumir2-libs/stdlib/kumirstdlib.hpp>
#include "enums/enums.h"
#include "entities/Instruction.h"


namespace Arduino {

typedef std::pair<uint32_t, uint16_t> AS_Key;
    // module | algorithm, id
typedef std::map<AS_Key, std::string> AS_Values;
struct AS_Helpers {
    AS_Values globals;
    AS_Values locals;
    AS_Values constants;
    AS_Values algorithms;
};

} // end namespace Arduino

#endif // ARDUINOGENERATOR_INSTRUCTION_H
