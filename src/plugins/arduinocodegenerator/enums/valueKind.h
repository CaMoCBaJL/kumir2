//
// Created by anton on 7/17/22.
//

#ifndef KUMIR2_VALUEKIND_H
#define KUMIR2_VALUEKIND_H
namespace Arduino{
    enum ValueKind {
        VK_Plain    = 0x00,
        VK_In       = 0x01,
        VK_InOut    = 0x02,
        VK_Out      = 0x03
    };
}
#endif //KUMIR2_VALUEKIND_H
