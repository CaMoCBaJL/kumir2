#ifndef ARDUINOCODEGENERATOR_GENERATOR_H
#define ARDUINOCODEGENERATOR_GENERATOR_H

#include <QtCore>

#include <kumir2-libs/dataformats/ast.h>
#include <kumir2-libs/dataformats/ast_variable.h>
#include <kumir2-libs/dataformats/ast_algorhitm.h>
#include <kumir2-libs/dataformats/lexem.h>
#include <kumir2-libs/errormessages/errormessages.h>
#include <kumir2/generatorinterface.h>
#include "arduino_enums.hpp"
#include "arduino_instruction.hpp"
#include "arduino_tableelem.hpp"



namespace Arduino {
struct Data;
}

namespace AST {
struct Lexem;
}

namespace ArduinoCodeGenerator {
    struct ConstValue{
        QVariant value;
        QList<Arduino::ValueType> baseType;
        QString recordModuleName;
        QString recordClassLocalizedName;
        QByteArray recordClassAsciiName;
        quint8 dimension;
        inline bool operator==(const ConstValue & other) {
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
    QList<QVariant> GetConstantValues();
    private:
    void addTimesLoopWithoutVarHead (const AST::StatementPtr st, QList<Arduino::Instruction> & result);
    void addTimesLoopWithVarHead (const AST::StatementPtr st, QList<Arduino::Instruction> & result);
    int findArrSize(QPair<QSharedPointer<AST::Expression>, QSharedPointer<AST::Expression>> bounds);
    QList<Arduino::Instruction> getOperands(AST::ExpressionPtr expr);
    Arduino::Instruction parseConstOrVarExpr(AST::ExpressionPtr expr);
    Arduino::TableElem AddConstName(Arduino::Data & data, Kumir::String constName, uint16_t constId);
    quint16 constantValue(Arduino::ValueType type, quint8 dimension, const QVariant & value,
                          const QString & recordModule, const QString & recordClass
                          );
    quint16 constantValue(const QList<Arduino::ValueType> & type, quint8 dimension, const QVariant & value,
                          const QString & recordModule, const QString & recordClass
                          );
    void addKumirModule(int id, const AST::ModulePtr  mod);
    void addFunction(int id, int moduleId, Arduino::ElemType type, const AST::ModulePtr  mod, const AST::AlgorithmPtr  alg);
    void addInputArgumentsMainAlgorhitm(int moduleId, int algorhitmId, const AST::ModulePtr  mod, const AST::AlgorithmPtr  alg);

    QList<Arduino::Instruction> instructions(
        int modId, int algId, int level,
        const QList<AST::StatementPtr> & statements);

    void ERRORR(int modId, int algId, int level, const AST::StatementPtr  st, QList<Arduino::Instruction> & result);
    void ASSIGN(int modId, int algId, int level, const AST::StatementPtr  st, QList<Arduino::Instruction> & result);
    void ASSERT(int modId, int algId, int level, const AST::StatementPtr  st, QList<Arduino::Instruction> & result);
    void PAUSE_STOP(int modId, int algId, int level, const AST::StatementPtr  st, QList<Arduino::Instruction> & result);
    void INIT(int modId, int algId, int level, const AST::StatementPtr  st, QList<Arduino::Instruction> & result);
    void CALL_SPECIAL(int modId, int algId, int level, const AST::StatementPtr  st, QList<Arduino::Instruction> & result);
    void LOOP(int modId, int algId, int level, const AST::StatementPtr  st, QList<Arduino::Instruction> & result);
    void IFTHENELSE(int modId, int algId, int level, const AST::StatementPtr  st, QList<Arduino::Instruction> & result);
    void SWITCHCASEELSE(int modId, int algId, int level, const AST::StatementPtr  st, QList<Arduino::Instruction> & result);
    void BREAK(int modId, int algId, int level, const AST::StatementPtr  st, QList<Arduino::Instruction> & result);

    QList<Arduino::Instruction> calculate(int modId, int algId, int level, const AST::ExpressionPtr st);
    QList<Arduino::Instruction> innerCalculation(int modId, int algId, int level, const AST::ExpressionPtr st);

    void findVariable(int modId, int algId, const AST::VariablePtr  var, Arduino::VariableScope & scope, quint16 & id) const;
    static const AST::VariablePtr  returnValue(const AST::AlgorithmPtr  alg);
    void findFunction(const AST::AlgorithmPtr  alg, quint8 & module, quint16 & id) ;


    static QList<Arduino::ValueType> valueType(const AST::Type & t);
    static Arduino::ValueKind valueKind(AST::VariableAccessType t);
    static Arduino::InstructionType operation(AST::ExpressionOperator op);

    AST::DataPtr ast_;
    Arduino::Data * byteCode_;
    QList< ConstValue > constants_;
    QList< QPair<quint8,quint16> > externs_;
    DebugLevel debugLevel_;
};
} // namespace ArduinoCodeGenerator
#endif // ARDUINOCODEGENERATOR_GENERATOR_H
