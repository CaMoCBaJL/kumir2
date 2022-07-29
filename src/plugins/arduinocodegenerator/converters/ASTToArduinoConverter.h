//
// Created by anton on 7/29/22.
//

#ifndef KUMIR2_ASTTOARDUINOCONVERTER_H
#define KUMIR2_ASTTOARDUINOCONVERTER_H
#include "../enums/enums.h"

namespace ArduinoCodeGenerator{
    Arduino::ValueType parseConstType(AST::ExpressionPtr constant){
        switch (constant->constant.type()) {
            case QMetaType::Void: return Arduino::VT_void;
            case QMetaType::Bool: return Arduino::VT_bool;
            case QMetaType::Int: return Arduino::VT_int;
            case QMetaType::Float:
            case QMetaType::Double: return Arduino::VT_float;
            case QMetaType::QChar:
            case QMetaType::Char: return Arduino::VT_char;
            case QMetaType::QString: return Arduino::VT_string;
            case QMetaType::User: return Arduino::VT_struct;
            default: return Arduino::VT_None;
        }
    }

    Arduino::InstructionType parseInstructionType(AST::ExpressionPtr st) {
        switch (st->operatorr) {
            case AST::OpSumm:
                return Arduino::SUM;
            case AST::OpSubstract:
                return Arduino::SUB;
            case AST::OpMultiply:
                return Arduino::MUL;
            case AST::OpDivision:
                return Arduino::DIV;
            case AST::OpPower:
                return Arduino::POW;
            case AST::OpNot:
                return Arduino::NEG;
            case AST::OpAnd:
                return Arduino::AND;
            case AST::OpOr:
                return Arduino::OR;
            case AST::OpEqual:
                return Arduino::EQ;
            case AST::OpNotEqual:
                return Arduino::NEQ;
            case AST::OpLess:
                return Arduino::LS;
            case AST::OpGreater:
                return Arduino::GT;
            case AST::OpLessOrEqual:
                return Arduino::LEQ;
            case AST::OpGreaterOrEqual:
                return Arduino::GEQ;
            default:
                return Arduino::CONST;
        }
    }

    Arduino::InstructionType Generator::parseOperationType(AST::ExpressionOperator op) {
        if (op == AST::OpSumm)
            return Arduino::SUM;
        else if (op == AST::OpSubstract)
            return Arduino::SUB;
        else if (op == AST::OpMultiply)
            return Arduino::MUL;
        else if (op == AST::OpDivision)
            return Arduino::DIV;
        else if (op == AST::OpPower)
            return Arduino::POW;
        else if (op == AST::OpNot)
            return Arduino::NEG;
        else if (op == AST::OpAnd)
            return Arduino::AND;
        else if (op == AST::OpOr)
            return Arduino::OR;
        else if (op == AST::OpEqual)
            return Arduino::EQ;
        else if (op == AST::OpNotEqual)
            return Arduino::NEQ;
        else if (op == AST::OpLess)
            return Arduino::LS;
        else if (op == AST::OpGreater)
            return Arduino::GT;
        else if (op == AST::OpLessOrEqual)
            return Arduino::LEQ;
        else if (op == AST::OpGreaterOrEqual)
            return Arduino::GEQ;
        else
            return Arduino::NOP;
    }

    Arduino::ValueKind Generator::parseValueKind(AST::VariableAccessType t) {
        if (t == AST::AccessArgumentIn)
            return Arduino::VK_In;
        else if (t == AST::AccessArgumentOut)
            return Arduino::VK_Out;
        else if (t == AST::AccessArgumentInOut)
            return Arduino::VK_InOut;
        else
            return Arduino::VK_Plain;
    }

    QList <Arduino::ValueType> Generator::parseValueType(const AST::Type &t) {
        QList <Arduino::ValueType> result;
        if (t.kind == AST::TypeInteger)
            result << Arduino::VT_int;
        else if (t.kind == AST::TypeReal)
            result << Arduino::VT_float;
        else if (t.kind == AST::TypeBoolean)
            result << Arduino::VT_bool;
        else if (t.kind == AST::TypeString)
            result << Arduino::VT_string;
        else if (t.kind == AST::TypeCharect)
            result << Arduino::VT_char;
        else if (t.kind == AST::TypeUser) {
            result << Arduino::VT_struct;
            for (int i = 0; i < t.userTypeFields.size(); i++) {
                AST::Field field = t.userTypeFields[i];
                result << parseValueType(field.second);
            }
        } else
            result << Arduino::VT_void;
        return result;
    }

    Arduino::ValueType parseVarType(AST::VariablePtr variable){
        switch (variable->baseType.kind) {
            case AST::TypeInteger:
                return Arduino::VT_int;
            case AST::TypeBoolean:
                return Arduino::VT_bool;
            case AST::TypeCharect:
                return Arduino::VT_char;
            case AST::TypeReal:
                return Arduino::VT_float;
            case AST::TypeString:
                return Arduino::VT_string;
            case AST::TypeUser:
                return Arduino::VT_struct;
            case AST::TypeNone:
                return Arduino::VT_void;
            default:
                return Arduino::VT_void;
        }
    }

    Arduino::Instruction Generator::parseConstOrVarExpr(AST::ExpressionPtr expr){
        Arduino::Instruction instr;

        if (expr->variable){
            instr.type = Arduino::VAR;
            instr.varName = expr->variable->name;
            instr.varType = Arduino::VT_None;
        } else{
            instr.type = Arduino::CONST;
            instr.arg = calculateConstantValue(Arduino::VT_string, 0, expr->constant, QString(), QString());
        }

        return instr;
    }

    static QString typeToSignature(const AST::Type &t) {
        QString result;
        if (t.kind == AST::TypeNone)
            result = "void";
        else if (t.kind == AST::TypeBoolean)
            result = "bool";
        else if (t.kind == AST::TypeInteger)
            result = "int";
        else if (t.kind == AST::TypeReal)
            result = "float";
        else if (t.kind == AST::TypeCharect)
            result = "char";
        else if (t.kind == AST::TypeString)
            result = "String";
        else if (t.kind == AST::TypeUser) {
            result = "struct {";
            for (int i = 0; i < t.userTypeFields.size(); i++) {
                if (i > 0)
                    result += ";";
                result += typeToSignature(t.userTypeFields.at(i).second);
            }
            result += "}";
        }
        Q_ASSERT(result.length() > 0);
        return result;
    }
}
#endif //KUMIR2_ASTTOARDUINOCONVERTER_H
