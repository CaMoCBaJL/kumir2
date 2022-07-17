//
// Created by anton on 7/17/22.
//

#ifndef ARDUINO_COMMON_HPP
#define ARDUINO_COMMON_HPP
#include "../enums/enums.h"
#include <stdint.h>
#include <QVariant>
#include <QString>
#include <QList>
#include "../entities/ConstValue.h"
#include "../entities/Instruction.h"
#include "kumir2-libs/dataformats/ast_statement.h"

namespace ArduinoCodeGenerator {

    class CommonMethodsProvider {
    public:
        CommonMethodsProvider();

        quint16 constantValue(Arduino::ValueType type, quint8 dimension, const QVariant &value,
                                     const QString &recordModule, const QString &recordClass,
                                     QList <ConstValue> &constants
        );

        quint16 constantValue(const QList <Arduino::ValueType> &type, quint8 dimension, const QVariant &value,
                                     const QString &recordModule, const QString &recordClass,
                                     QList <ConstValue> &constants
        );

        QList <Arduino::Instruction> instructions(int modId, int algId, int level,
                                                  const QList <AST::StatementPtr> &statements);
    };

} // ArduinoCodeGenerator

#endif //ARDUINO_COMMON_HPP
