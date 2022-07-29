//
// Created by anton on 7/17/22.
//

#ifndef KUMIR2_ELEMTYPE_H
#define KUMIR2_ELEMTYPE_H
namespace Arduino{
    enum ElemType {
        EL_NONE     = 0x00,     // stream delimeter
        EL_LOCAL    = 0x01,       // Local variable
        EL_GLOBAL   = 0x02,      // Global variable
        EL_CONST    = 0x03,       // Constant
        EL_FUNCTION = 0x04,    // Local defined function
        EL_EXTERN   = 0x05,      // External module name
        EL_INIT     = 0x06,     // Local defined module initializer
        EL_MAIN     = 0x07,     // Main (usually - first) function
        EL_TESTING  = 0x08,      // Testing function
        EL_BELOWMAIN= 0x09,      // Function evaluated below main function
        EL_EXTERN_INIT = 0x0a
    };

}
#endif //KUMIR2_ELEMTYPE_H
