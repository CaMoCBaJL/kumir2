//
// Created by anton on 7/17/22.
//

#ifndef KUMIR2_DATA_H
#define KUMIR2_DATA_H
namespace Arduino{
    struct Data {
        std::deque <TableElem> d;
        uint8_t versionMaj;
        uint8_t versionMin;
        uint8_t versionRel;
        unsigned long lastModified;
    };
}
#endif //KUMIR2_DATA_H
