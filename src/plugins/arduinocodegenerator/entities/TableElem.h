//
// Created by anton on 7/17/22.
//

#ifndef KUMIR2_TABLEELEM_H
#define KUMIR2_TABLEELEM_H
#include "../enums/enums.h"
#include "Instruction.h"
namespace Arduino {

    using Kumir::String;
    using Arduino::CVariable;
    using Arduino::ElemType;
    using Arduino::ValueType;
    using Arduino::ValueKind;

    struct TableElem {
        ElemType type; // Element type
        std::list <ValueType> vtype; // Value type
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
        std::vector <Instruction> instructions; // for local defined function
        inline TableElem() {
            type = Arduino::EL_NONE;
            vtype.push_back(Arduino::VT_void);
            dimension = 0;
            refvalue = Arduino::VK_Plain;
            module = 0;
            algId = id = 0;
        }
    };
}
#endif //KUMIR2_TABLEELEM_H
