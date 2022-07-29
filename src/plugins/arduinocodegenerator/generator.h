#ifndef ARDUINOCODEGENERATOR_GENERATOR_H
#define ARDUINOCODEGENERATOR_GENERATOR_H

#include <QtCore>

#include <kumir2-libs/dataformats/ast.h>
#include <kumir2-libs/dataformats/ast_variable.h>
#include <kumir2-libs/dataformats/ast_algorhitm.h>
#include <kumir2-libs/dataformats/lexem.h>
#include <kumir2-libs/errormessages/errormessages.h>
#include <kumir2/generatorinterface.h>
#include "arduino_instruction.hpp"
#include "arduino_tableelem.hpp"
#include "entities/ConstValue.h"

namespace Arduino {
struct Data;
}

namespace AST {
struct Lexem;
}

namespace ArduinoCodeGenerator{
    typedef Shared::GeneratorInterface::DebugLevel DebugLevel;

    class Generator : public QObject
{

    Q_OBJECT
public:
    explicit Generator(QObject *parent = 0);
    void reset(const AST::DataPtr ast, Arduino::Data * bc);
    void addModule(const AST::ModulePtr mod);
    void generateExternTable();
    void setDebugLevel(DebugLevel debugLevel);
    QList<QVariant> getConstantValues();
    private:
#pragma region loopGeneration
    void addTimesLoopWithoutVarHead (const AST::StatementPtr st, QList<Arduino::Instruction> & result);
    void addTimesLoopWithVarHead (const AST::StatementPtr st, QList<Arduino::Instruction> & result);
    void generateWhileLoopHeader(int modId, int algId,
                                 int level,
                                 const AST::StatementPtr st,
                                 QList<Arduino::Instruction> &result);
    void generateForLoopHeader(int modId, int algId,
                               int level,
                               const AST::StatementPtr st,
                               QList<Arduino::Instruction> &result);
    void generateTimesLoopHeader(int modId, int algId,
                                 int level,
                                 const AST::StatementPtr st,
                                 QList<Arduino::Instruction> &result);
    Arduino::InstructionType getForLoopValuesRelation(int fromValue, int toValue);
    Arduino::Instruction getForLoopVariable(AST::ExpressionPtr & ex);
    void addForLoopStep(AST::ExpressionPtr & ex, QList<Arduino::Instruction> &instructions, int fromValue, int toValue);
    void generateLoopHead(int modId, int algId,
                          int level,
                          const AST::StatementPtr st,
                          QList<Arduino::Instruction> &result);
    void generateLoopBody(int modId, int algId,
                          int level,
                          const AST::StatementPtr st,
                          QList<Arduino::Instruction> &result);
    void generateLoopTail(int modId, int algId,
                                     int level,
                                     const AST::StatementPtr st,
                                     QList<Arduino::Instruction> &result);
#pragma endregion
#pragma region statement calculations
    int findArrSize(QPair<QSharedPointer<AST::Expression>, QSharedPointer<AST::Expression>> bounds);
    QList<Arduino::Instruction> additionalSubExpressionCalculations(AST::ExpressionPtr expr);
    QList<Arduino::Instruction> calculateStatement(int modId, int algId, int level, const AST::ExpressionPtr st);
    QList<Arduino::Instruction> parseStatement(int modId, int algId, int level, const AST::ExpressionPtr st);
#pragma endregion
    Arduino::Instruction parseConstOrVarExpr(AST::ExpressionPtr expr);
    quint16 calculateConstantValue(Arduino::ValueType type, quint8 dimension, const QVariant & value,
                          const QString & recordModule, const QString & recordClass
                          );
    quint16 calculateConstantValue(const QList<Arduino::ValueType> & type, quint8 dimension, const QVariant & value,
                          const QString & recordModule, const QString & recordClass
                          );
    void addKumirModule(int id, const AST::ModulePtr  mod);
    void addFunction(int id, int moduleId, Arduino::ElemType type, const AST::ModulePtr  mod, const AST::AlgorithmPtr  alg);
    void addInputArgumentsMainAlgorhitm(int moduleId, int algorhitmId, const AST::ModulePtr  mod, const AST::AlgorithmPtr  alg);

    QList<Arduino::Instruction> instructions(
        int modId, int algId, int level,
        const QList<AST::StatementPtr> & statements);
#pragma region main generation methods
    void generateErrorInstruction(int modId, int algId, int level, const AST::StatementPtr  st, QList<Arduino::Instruction> & result);
    void generateAssignInstruction(int modId, int algId, int level, const AST::StatementPtr  st, QList<Arduino::Instruction> & result);
    void generateAssertInstruction(int modId, int algId, int level, const AST::StatementPtr  st, QList<Arduino::Instruction> & result);
    void generateBreakInstruction(int modId, int algId, int level, const AST::StatementPtr  st, QList<Arduino::Instruction> & result);
    void generateVarInitInstruction(int modId, int algId, int level, const AST::StatementPtr  st, QList<Arduino::Instruction> & result);
    void generateTerminalIOInstruction(int modId, int algId, int level, const AST::StatementPtr  st, QList<Arduino::Instruction> & result);
    void generateConditionInstruction(int modId, int algId, int level, const AST::StatementPtr  st, QList<Arduino::Instruction> & result);
    void generateChoiceInstruction(int modId, int algId, int level, const AST::StatementPtr  st, QList<Arduino::Instruction> & result);
    void generateLoopInstruction(int modId, int algId, int level, const AST::StatementPtr  st, QList<Arduino::Instruction> & result);
#pragma endregion
    void findVariable(int modId, int algId, const AST::VariablePtr  var, Arduino::VariableScope & scope, quint16 & id) const;
    static const AST::VariablePtr generateReturnValue(const AST::AlgorithmPtr  alg);
    void findFunction(const AST::AlgorithmPtr  alg, quint8 & module, quint16 & id) ;


    static QList<Arduino::ValueType> parseValueType(const AST::Type & t);
    static Arduino::ValueKind parseValueKind(AST::VariableAccessType t);
    static Arduino::InstructionType parseOperationType(AST::ExpressionOperator op);

    AST::DataPtr ast_;
    Arduino::Data * byteCode_;
    QList< ConstValue > constants_;
    QList< QPair<quint8,quint16> > externs_;
    DebugLevel debugLevel_;
    int loopIteratorCounter_;
};
} // namespace ArduinoCodeGenerator
#endif // ARDUINOCODEGENERATOR_GENERATOR_H
