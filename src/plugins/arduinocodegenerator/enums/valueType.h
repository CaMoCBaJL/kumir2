//
// Created by anton on 7/17/22.
//

#ifndef KUMIR2_VALUETYPE_H
#define KUMIR2_VALUETYPE_H
namespace Arduino {
    enum ValueType {
        VT_void = 0x00,
        VT_int = 0x01,
        VT_float = 0x02,
        VT_char = 0x03,
        VT_bool = 0x04,
        VT_string = 0x05,
        VT_struct = 0xFF,
        VT_None = -1
    };
}
#endif //KUMIR2_VALUETYPE_H
