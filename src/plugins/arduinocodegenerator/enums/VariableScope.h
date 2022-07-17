//
// Created by anton on 7/17/22.
//

#ifndef KUMIR2_VARIABLESCOPE_H
#define KUMIR2_VARIABLESCOPE_H
namespace Arduino {

    enum VariableScope {
        UNDEF = 0x00, // Undefined if not need
        CONSTT = 0x01, // Value from constants table
        LOCAL = 0x02, // Value from locals table
        GLOBAL = 0x03  // Value from globals table
    };
}
#endif //KUMIR2_VARIABLESCOPE_H
