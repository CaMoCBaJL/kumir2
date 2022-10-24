//
// Created by anton on 7/29/22.
//

#ifndef KUMIR2_HELPERS_H
#define KUMIR2_HELPERS_H
namespace Arduino{
    typedef std::pair<uint32_t, uint16_t> AS_Key;
    // module | algorithm, id
    typedef std::map<AS_Key, std::string> AS_Values;
    struct AS_Helpers {
        AS_Values globals;
        AS_Values locals;
        AS_Values constants;
        AS_Values algorithms;
    };
}
#endif //KUMIR2_HELPERS_H
