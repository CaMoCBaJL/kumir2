//
// Created by anton on 7/17/22.
//

#ifndef KUMIR2_LINESPECIFICATION_H
#define KUMIR2_LINESPECIFICATION_H
namespace Arduino{
    enum LineSpecification {
        LINE_NUMBER = 0x00,
        COLUMN_START_AND_END = 0x80
    };
}
#endif //KUMIR2_LINESPECIFICATION_H
