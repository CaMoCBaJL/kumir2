//
// Created by anton on 7/17/22.
//

#ifndef ARDUINO_LOOPGENERATOR_H
#define ARDUINO_LOOPGENERATOR_H
#include "../../entities/Instruction.h"
#include "kumir2-libs/dataformats/ast_statement.h"
#include "../../entities/ConstValue.h"
#include "../CommonMethodsProvider.hpp"
#include "../ExpressionCalculator.hpp"
#include <QList>
namespace ArduinoCodeGenerator {
    class LoopGenerator {
    public:
        LoopGenerator(QList<ConstValue> & constants);

        void
        generateLoop(int modId, int algId, int level, const AST::StatementPtr st, QList <Arduino::Instruction> &result);

    private:
        void addTimesLoopWithoutVarHead(const AST::StatementPtr st, QList <Arduino::Instruction> &result);

        void addTimesLoopWithVarHead(const AST::StatementPtr st, QList <Arduino::Instruction> &result);

        void
        addForLoopStep(AST::ExpressionPtr &ex, QList <Arduino::Instruction> &instructions, int fromValue, int toValue);

        Arduino::Instruction getForLoopVariable(AST::ExpressionPtr &ex);

        Arduino::InstructionType getForLoopValuesRelation(int fromValue, int toValue);

        int loopIteratorCounter_;
        QList<ConstValue> constants_;
        CommonMethodsProvider provider_;
        ExpressionCalculator calculator_;
    };
}

#endif //ARDUINO_LOOPGENERATOR_H
