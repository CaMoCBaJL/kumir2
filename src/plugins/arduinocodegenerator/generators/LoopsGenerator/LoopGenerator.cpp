//
// Created by anton on 7/17/22.
//

#include "LoopGenerator.hpp"
#include "../../enums/enums.h"
#include "../../entities/Instruction.h"
#include "../../entities/ConstValue.h"
#include <QString>
#include "QList"
#include "kumir2-libs/dataformats/ast_expression.h"
#include "kumir2-libs/dataformats/ast_variable.h"
#include "kumir2-libs/errormessages/errormessages.h"
#include "../CommonMethodsProvider.hpp"
#include "../ExpressionCalculator.hpp"

namespace ArduinoCodeGenerator{
    using namespace Shared;

    LoopGenerator::LoopGenerator(QList<ConstValue> & constants) {
        loopIteratorCounter_ = 0;
        this->constants_ = constants;
        this->provider_ = CommonMethodsProvider();
        this->calculator_ = ExpressionCalculator();
    }

    Arduino::InstructionType LoopGenerator::getForLoopValuesRelation(int fromValue, int toValue){
        if (fromValue > toValue){
            return Arduino::GT;
        } else if (fromValue >= toValue){
            return Arduino::GEQ;
        } else if (fromValue < toValue){
            return Arduino::LS;
        } else if (fromValue <= toValue){
            return Arduino::LEQ;
        }

        return Arduino::NEQ;
    }

    Arduino::Instruction LoopGenerator::getForLoopVariable(AST::ExpressionPtr & ex) {
        Arduino::Instruction variable;

        if (ex->variable == nullptr) {
            variable.registerr = ex->constant.toInt();
            variable.type = Arduino::CONST;
        } else {
            variable.varName = ex->variable->name;
            variable.registerr = ex->variable->initialValue.toInt();
            variable.type = Arduino::VAR;
        }

        return variable;
    }

    void LoopGenerator::addForLoopStep(AST::ExpressionPtr & ex, QList<Arduino::Instruction> &instructions, int fromValue, int toValue) {
        Arduino::Instruction step;

        if(ex == nullptr){
            if (fromValue > toValue){
                step.type = Arduino::DCR;
            } else {
                step.type = Arduino::INC;
            }
            instructions << step;

            return;
        }
        if (ex->variable == nullptr){
            step.registerr = ex->constant.toInt();
        }
        else{
            step.registerr = ex->variable->initialValue.toInt();
        }

        if (step.registerr > 0){
            if (abs(step.registerr) == 1){
                step.type = Arduino::INC;
            } else {
                step.type = Arduino::SUM;
            }
        } else {
            if (abs(step.registerr) == 1){
                step.type = Arduino::DCR;
            } else {
                step.type = Arduino::SUB;
            }
        }

        instructions << step;
    }

    void LoopGenerator::addTimesLoopWithVarHead (const AST::StatementPtr st, QList<Arduino::Instruction> & result){
        Arduino::Instruction timesVar;
        Arduino::Instruction instrBuffer;

        timesVar.type = Arduino::VAR;
        timesVar.varName = st->loop.timesValue->variable->name;
        result << timesVar;

        instrBuffer.type = Arduino::END_VAR;
        result << instrBuffer;

        timesVar.varType = Arduino::VT_None;
        result << timesVar;
        if (st->loop.timesValue->variable->initialValue > 0){
            instrBuffer.type = Arduino::GT;
            result << instrBuffer;
        } else {
            instrBuffer.type = Arduino::LS;
            result << instrBuffer;
        }

        instrBuffer.type = Arduino::CONST;
        instrBuffer.arg = provider_.constantValue(Arduino::VT_int, 0, st->loop.timesValue->variable->initialValue, QString(),
                                        QString(), this->constants_);
        result << instrBuffer;

        instrBuffer.type = Arduino::END_VAR;
        result << instrBuffer;

        result << timesVar;

        if (st->loop.timesValue->variable->initialValue > 0){
            instrBuffer.type = Arduino::INC;
            result << instrBuffer;
        } else {
            instrBuffer.type = Arduino::DCR;
            result << instrBuffer;
        }

        instrBuffer.type = Arduino::END_VAR;
        result << instrBuffer;
    }

    void LoopGenerator::addTimesLoopWithoutVarHead (const AST::StatementPtr st, QList<Arduino::Instruction> & result) {
        Arduino::Instruction timesVar;
        Arduino::Instruction instrBuffer;

        timesVar.type = Arduino::VAR;
        timesVar.varName = QString::fromStdString("loopIterator" + std::to_string(loopIteratorCounter_));
        timesVar.varType = Arduino::VT_int;
        result << timesVar;

        loopIteratorCounter_++;

        instrBuffer.type = Arduino::ASG;
        result << instrBuffer;

        instrBuffer.type = Arduino::CONST;
        instrBuffer.arg = provider_.constantValue(Arduino::VT_int, 0, 0, QString(), QString(), this->constants_);
        result << instrBuffer;

        instrBuffer.type = Arduino::END_VAR;
        result << instrBuffer;

        timesVar.varType = Arduino::VT_None;
        result << timesVar;

        instrBuffer.type = Arduino::LS;
        result << instrBuffer;

        instrBuffer.type = Arduino::CONST;
        instrBuffer.arg = provider_.constantValue(Arduino::VT_int, 0, st->loop.timesValue->constant.toInt(), QString(),
                                        QString(), this->constants_);
        result << instrBuffer;

        instrBuffer.type = Arduino::END_VAR;
        result << instrBuffer;

        result << timesVar;

        instrBuffer.type = Arduino::INC;
        result << instrBuffer;

        instrBuffer.type = Arduino::END_VAR;
        result << instrBuffer;
    }

    void LoopGenerator::generateLoop(int modId, int algId,
                         int level,
                         const AST::StatementPtr st,
                         QList<Arduino::Instruction> &result) {
        if (st->beginBlockError.size() > 0) {
            const QString error = ErrorMessages::message("KumirAnalizer", QLocale::Russian, st->beginBlockError);
            Arduino::Instruction err;
            err.type = Arduino::ERRORR;
            err.scope = Arduino::CONSTT;
            err.arg = provider_.constantValue(Arduino::VT_string, 0, error, QString(), QString(), this->constants_);
            result << err;
            return;
        }

        if ((st->loop.type == AST::LoopWhile || st->loop.type == AST::LoopForever) && !st->loop.endCondition) {
            Arduino::Instruction instr;
            instr.type = Arduino::WhileLoop;
            result << instr;

            instr.type = Arduino::START_SUB_EXPR;
            result << instr;

            if (st->loop.whileCondition) {
                QList <Arduino::Instruction> whileCondInstructions = calculator_.calculate(modId, algId, level,
                                                                               st->loop.whileCondition);
                result << whileCondInstructions;
            } else {
                instr.type = Arduino::CONST;
                instr.arg = provider_.constantValue(Arduino::VT_bool, 0, true, QString(), QString(), this->constants_);
                result << instr;
            }
        } else if (st->loop.type == AST::LoopTimes) {
            if (st->loop.timesValue->constant.toInt() < 1){
                return;
            }

            Arduino::Instruction instrBuffer;
            Arduino::Instruction timesVar;

            instrBuffer.type = Arduino::ForLoop;
            result << instrBuffer;

            instrBuffer.type = Arduino::START_SUB_EXPR;
            result << instrBuffer;

            if (st->loop.timesValue->variable) {
                addTimesLoopWithVarHead(st, result);
            } else {
                addTimesLoopWithoutVarHead(st, result);
            }

        } else if (st->loop.type == AST::LoopFor) {
            Arduino::Instruction instrBuffer;
            Arduino::Instruction workCondition;
            Arduino::Instruction operation;

            instrBuffer.type = Arduino::ForLoop;
            result << instrBuffer;

            instrBuffer.type = Arduino::START_SUB_EXPR;
            result << instrBuffer;

            workCondition.type = Arduino::VAR;
            workCondition.varName = st->loop.forVariable->name;
            result << workCondition;

            operation.type = Arduino::ASG;
            result << operation;

            Arduino::Instruction fromValue = getForLoopVariable(st->loop.fromValue);
            result << fromValue;

            workCondition.type = Arduino::END_VAR;
            result << workCondition;

            workCondition.type = Arduino::VAR;
            result << workCondition;

            Arduino::Instruction toValue = getForLoopVariable(st->loop.toValue);

            operation.type = getForLoopValuesRelation(fromValue.registerr, toValue.registerr);
            result << operation;

            result << toValue;

            workCondition.type = Arduino::END_VAR;
            result << workCondition;

            workCondition.type = Arduino::VAR;
            result << workCondition;

            addForLoopStep(st->loop.stepValue, result, result.at(result.size() - 2).registerr,
                           result.at(result.size() - 1).registerr);
        }

        if (st->loop.endCondition){
            Arduino::Instruction beginDo;
            beginDo.type = Arduino::DO;
            result << beginDo;
        } else {
            Arduino::Instruction endLoopHead;
            endLoopHead.type = Arduino::END_ST_HEAD;
            result << endLoopHead;
        }

        QList <Arduino::Instruction> instrs = provider_.instructions(modId, algId, level, st->loop.body);
        result += instrs;

        bool endsWithError = st->endBlockError.length() > 0;
        if (endsWithError) {
            const QString error = ErrorMessages::message("KumirAnalizer", QLocale::Russian, st->endBlockError);
            Arduino::Instruction ee;
            ee.type = Arduino::ERRORR;
            ee.scope = Arduino::CONSTT;
            ee.arg = provider_.constantValue(Arduino::VT_string, 0, error, QString(), QString(), this->constants_);
            result << ee;
            return;
        }

        Arduino::Instruction endLoop;
        endLoop.type = Arduino::END_ST;
        result << endLoop;

        if (st->loop.endCondition) {
            Arduino::Instruction endCondition;
            endCondition.type = Arduino::WhileLoop;
            result << endCondition;
            endCondition.type = Arduino::START_SUB_EXPR;
            result << endCondition;

            QList <Arduino::Instruction> endCondInstructions = calculator_.calculate(modId, algId, level, st->loop.endCondition);
            result << endCondInstructions;

            endCondition.type = Arduino::END_SUB_EXPR;
            result << endCondition;
        }
    }
}
