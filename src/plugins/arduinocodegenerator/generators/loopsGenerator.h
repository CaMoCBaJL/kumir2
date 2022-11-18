//
// Created by anton on 7/29/22.
//

#ifndef KUMIR2_LOOPGENERATOR_H
#define KUMIR2_LOOPGENERATOR_H

namespace ArduinoCodeGenerator{
    using namespace Shared;
    Arduino::InstructionType Generator::getForLoopValuesRelation(int fromValue, int toValue){
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

    Arduino::Instruction Generator::getForLoopVariable(AST::ExpressionPtr & ex) {
        Arduino::Instruction variable;

        if (ex->variable == nullptr) {
            variable.registerr = ex->constant.toInt();
            variable.arg = -1;
            variable.type = Arduino::CONST;
        } else {
            variable.varName = ex->variable->name;
            variable.registerr = ex->variable->initialValue.toInt();
            variable.type = Arduino::VAR;
        }

        return variable;
    }

    void Generator::addForLoopStep(AST::ExpressionPtr & ex, QList<Arduino::Instruction> &instructions, int fromValue, int toValue) {
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

    void Generator::addTimesLoopWithVarHead (const AST::StatementPtr st, QList<Arduino::Instruction> & result){
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
        instrBuffer.arg = calculateConstantValue(Arduino::VT_int, 0, st->loop.timesValue->variable->initialValue, QString(), QString());
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

    void Generator::addTimesLoopWithoutVarHead (const AST::StatementPtr st, QList<Arduino::Instruction> & result) {
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
        instrBuffer.arg = calculateConstantValue(Arduino::VT_int, 0, 0, QString(), QString());
        result << instrBuffer;

        instrBuffer.type = Arduino::END_VAR;
        result << instrBuffer;

        timesVar.varType = Arduino::VT_None;
        result << timesVar;

        instrBuffer.type = Arduino::LS;
        result << instrBuffer;

        instrBuffer.type = Arduino::CONST;
        instrBuffer.arg = calculateConstantValue(Arduino::VT_int, 0, st->loop.timesValue->constant.toInt(), QString(),
                                                 QString());
        result << instrBuffer;

        instrBuffer.type = Arduino::END_VAR;
        result << instrBuffer;

        result << timesVar;

        instrBuffer.type = Arduino::INC;
        result << instrBuffer;

        instrBuffer.type = Arduino::END_VAR;
        result << instrBuffer;
    }

    void Generator::generateWhileLoopHeader(int modId, int algId,
                                            int level,
                                            const AST::StatementPtr st,
                                            QList<Arduino::Instruction> &result) {
        if ((st->loop.type == AST::LoopWhile || st->loop.type == AST::LoopForever) && !st->loop.endCondition) {
            Arduino::Instruction instr;
            instr.type = Arduino::WhileLoop;
            result << instr;

            instr.type = Arduino::START_SUB_EXPR;
            result << instr;

            if (st->loop.whileCondition) {
                QList <Arduino::Instruction> whileCondInstructions = calculateStatement(modId, algId, level,
                                                                               st->loop.whileCondition);
                result << whileCondInstructions;
            } else {
                instr.type = Arduino::CONST;
                instr.arg = calculateConstantValue(Arduino::VT_bool, 0, true, QString(), QString());
                result << instr;
            }
        }
    }

    void Generator::generateForLoopHeader(int modId, int algId,
                                          int level,
                                          const AST::StatementPtr st,
                                          QList<Arduino::Instruction> &result){
        Arduino::Instruction instrBuffer;
        Arduino::Instruction workCondition;
        Arduino::Instruction parseOperationType;

        instrBuffer.type = Arduino::ForLoop;
        result << instrBuffer;

        instrBuffer.type = Arduino::START_SUB_EXPR;
        result << instrBuffer;

        workCondition.type = Arduino::VAR;
        workCondition.varName = st->loop.forVariable->name;
        workCondition.varType = parseVarType(st->loop.forVariable);
        result << workCondition;

        parseOperationType.type = Arduino::ASG;
        result << parseOperationType;

        Arduino::Instruction fromValue = getForLoopVariable(st->loop.fromValue);
        result << fromValue;

        workCondition.type = Arduino::END_VAR;
        result << workCondition;

        workCondition.type = Arduino::VAR;
        result << workCondition;

        Arduino::Instruction toValue = getForLoopVariable(st->loop.toValue);

        parseOperationType.type = getForLoopValuesRelation(fromValue.registerr, toValue.registerr);
        result << parseOperationType;

        result << toValue;

        workCondition.type = Arduino::END_VAR;
        result << workCondition;

        workCondition.type = Arduino::VAR;
        result << workCondition;

        addForLoopStep(st->loop.stepValue, result, result.at(result.size() - 2).registerr,
                       result.at(result.size() - 1).registerr);
    }

    void Generator::generateTimesLoopHeader(int modId, int algId,
                                            int level,
                                            const AST::StatementPtr st,
                                            QList<Arduino::Instruction> &result){
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
    }

    void Generator::generateLoopHead(int modId, int algId,
                                     int level,
                                     const AST::StatementPtr st,
                                     QList<Arduino::Instruction> &result){
        if (st->beginBlockError.size() > 0) {
            const QString error = ErrorMessages::message("KumirAnalizer", QLocale::Russian, st->beginBlockError);
            Arduino::Instruction err;
            err.type = Arduino::ERRORR;
            err.scope = Arduino::CONSTT;
            err.arg = calculateConstantValue(Arduino::VT_string, 0, error, QString(), QString());
            result << err;
            return;
        }

        switch (st->loop.type) {
            case AST::LoopForever:
            case AST::LoopWhile:
                generateWhileLoopHeader(modId, algId, level, st, result);
                break;
            case AST::LoopFor:
                generateForLoopHeader(modId, algId, level, st, result);
                break;
            case AST::LoopTimes:
                generateTimesLoopHeader(modId, algId, level, st, result);
                break;
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
    }

    void Generator::generateLoopBody(int modId, int algId,
                                     int level,
                                     const AST::StatementPtr st,
                                     QList<Arduino::Instruction> &result){
        QList <Arduino::Instruction> instrs = calculateInstructions(modId, algId, level, st->loop.body);
        result += instrs;

        bool endsWithError = st->endBlockError.length() > 0;
        if (endsWithError) {
            const QString error = ErrorMessages::message("KumirAnalizer", QLocale::Russian, st->endBlockError);
            Arduino::Instruction ee;
            ee.type = Arduino::ERRORR;
            ee.scope = Arduino::CONSTT;
            ee.arg = calculateConstantValue(Arduino::VT_string, 0, error, QString(), QString());
            result << ee;
            return;
        }
    }

    void Generator::generateLoopTail(int modId, int algId,
                                     int level,
                                     const AST::StatementPtr st,
                                     QList<Arduino::Instruction> &result) {
        Arduino::Instruction endLoop;
        endLoop.type = Arduino::END_ST;
        result << endLoop;

        if (st->loop.endCondition) {
            Arduino::Instruction endCondition;
            endCondition.type = Arduino::WhileLoop;
            result << endCondition;
            endCondition.type = Arduino::START_SUB_EXPR;
            result << endCondition;

            QList <Arduino::Instruction> endCondInstructions = calculateStatement(modId, algId, level, st->loop.endCondition);
            result << endCondInstructions;

            endCondition.type = Arduino::END_SUB_EXPR;
            result << endCondition;
        }
    }

    void Generator::generateLoopInstruction(int modId, int algId,
                                            int level,
                                            const AST::StatementPtr st,
                                            QList<Arduino::Instruction> &result) {

        generateLoopHead(modId, algId, level, st, result);

        generateLoopBody(modId, algId, level, st, result);

        generateLoopTail(modId, algId, level, st, result);
    }
}
#endif //KUMIR2_LOOPGENERATOR_H
