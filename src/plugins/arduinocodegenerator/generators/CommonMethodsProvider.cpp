//
// Created by anton on 7/17/22.
//

#include "CommonMethodsProvider.hpp"
#include "../entities/Instruction.h"
#include "../enums/enums.h"
#include "kumir2-libs/dataformats/ast_statement.h"
#include <QList>
namespace ArduinoCodeGenerator {
    CommonMethodsProvider::CommonMethodsProvider() {

    }

    quint16 CommonMethodsProvider::constantValue(const QList <Arduino::ValueType> &type, quint8 dimension, const QVariant &value,
                          const QString &moduleName, const QString &className, QList< ConstValue > & constants
    ) {
        ConstValue c;
        c.baseType = type;
        c.dimension = dimension;
        c.value = value;
        c.recordModuleName = moduleName;
        c.recordClassLocalizedName = className;
        if (!constants.contains(c)) {
            constants << c;
            return constants.size() - 1;
        } else {
            return constants.indexOf(c);
        }
    }

    quint16 CommonMethodsProvider::constantValue(Arduino::ValueType type, quint8 dimension, const QVariant &value,
                                 const QString &moduleName, const QString &className, QList<ConstValue> & constants) {
        QList <Arduino::ValueType> vt;
        vt.push_back(type);
        return constantValue(vt, dimension, value, QString(), QString(), constants);
    }

    QList<Arduino::Instruction> CommonMethodsProvider::instructions(
            int modId, int algId, int level,
            const QList <AST::StatementPtr> &statements) {

        QList <Arduino::Instruction> result;
        for (int i = 0; i < statements.size(); i++) {
            const AST::StatementPtr st = statements[i];
            switch (st->type) {
                case AST::StError:
                    if (!st->skipErrorEvaluation)
                        ERRORR(modId, algId, level, st, result);
                    break;
                case AST::StAssign:
                    ASSIGN(modId, algId, level, st, result);
                    break;
                case AST::StAssert:
                    ASSERT(modId, algId, level, st, result);
                    break;
                case AST::StVarInitialize:
                    INIT(modId, algId, level, st, result);
                    break;
                case AST::StInput:
                    CALL_SPECIAL(modId, algId, level, st, result);
                    break;
                case AST::StOutput:
                    CALL_SPECIAL(modId, algId, level, st, result);
                    break;
                case AST::StLoop:
                    LoopGenerator(this->constants_);/*.generateLoop(modId, algId, level + 1, st, result);*/
                    break;
                case AST::StIfThenElse:
                    IFTHENELSE(modId, algId, level, st, result);
                    break;
                case AST::StSwitchCaseElse:
                    SWITCHCASEELSE(modId, algId, level, st, result);
                    break;
                case AST::StBreak:
                    BREAK(modId, algId, level, st, result);
                    break;
                case AST::StPause:
                    PAUSE_STOP(modId, algId, level, st, result);
                    break;
                case AST::StHalt:
                    PAUSE_STOP(modId, algId, level, st, result);
                    break;
                default:
                    break;
            }

            Arduino::Instruction delimiter;
            delimiter.type = Arduino::STR_DELIM;
            result << delimiter;
        }
        return result;
    }
}