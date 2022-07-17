//
// Created by anton on 7/17/22.
//

#ifndef ARDUINO_CONSTVALUE_H
#define ARDUINO_CONSTVALUE_H
#include "../enums/enums.h"
namespace ArduinoCodeGenerator{
    struct ConstValue {
        QVariant value;
        QList <Arduino::ValueType> baseType;
        QString recordModuleName;
        QString recordClassLocalizedName;
        QByteArray recordClassAsciiName;
        quint8 dimension;

        inline bool operator==(const ConstValue &other) {
            return
                    baseType == other.baseType &&
                    dimension == other.dimension &&
                    recordModuleName == other.recordModuleName &&
                    recordClassAsciiName == other.recordClassAsciiName &&
                    value == other.value;
        }

        inline ConstValue() {
            baseType.push_back(Arduino::VT_void);
            dimension = 0;
        }
    };
}
#endif //ARDUINO_CONSTVALUE_H
