//
// Created by anton on 7/29/22.
//

#ifndef KUMIR2_VARINITIALIZATIONGENERATOR_H
#define KUMIR2_VARINITIALIZATIONGENERATOR_H
namespace ArduinoCodeGenerator{
    void Generator::generateVarInitInstruction(int modId, int algId, int level, const AST::StatementPtr  st, QList<Arduino::Instruction> & result)
    {
        for (int i=0; i<st->variables.size(); i++) {
            const AST::VariablePtr var = st->variables[i];
            Arduino::Instruction instr;
            if (var->dimension > 0 && var->bounds.size()>0) {
                instr.type = Arduino::ARR;
                instr.varName = var->name;
                instr.varType = parseVarType(var);
                result << instr;
                for (int i=0; i< var->dimension; i++) {
                    instr.varName = nullptr;
                    instr.type = Arduino::CONST;
                    instr.arg = std::numeric_limits<uint16_t>::max();
                    instr.registerr = findArrSize(var->bounds[i]);
                    result << instr;
                    if (i + 1 < var->dimension) {
                        instr.type = Arduino::END_ARG;
                        result << instr;
                    }
                }
                instr.type = Arduino::END_ARR;
                result << instr;
                instr.type = Arduino::END_VAR;
                result << instr;
            }
            else if (var->dimension > 0 && var->bounds.size()==0) {
                // TODO : Implement me after compiler support
            }
            else {
                instr.type = Arduino::VAR;
                instr.varName = var->name;
                Arduino::ValueType varType = parseVarType(var);
                instr.varType = varType;
                result << instr;
                if (var->initialValue.type() != QMetaType::UnknownType) {
                    instr.type = Arduino::ASG;
                    result << instr;
                    instr.type = Arduino::CONST;
                    instr.varName = nullptr;
                    instr.arg = calculateConstantValue(varType, 0, var->initialValue, QString(), QString());
                    result << instr;
                }
                instr.type = Arduino::END_VAR;
                result << instr;
            }
        }
    }

    int Generator::findArrSize(QPair<QSharedPointer<AST::Expression>, QSharedPointer<AST::Expression>> bounds){
        int result = 0;
        quint16 indx = 0;

        if (!bounds.first->variable) {
            result = bounds.first->constant.toInt();
        }
        else{
            AST::VariablePtr var = bounds.first->variable;
            indx = calculateConstantValue(parseValueType(var->baseType), var->dimension, var->initialValue,
                                          var->baseType.actor ? var->baseType.actor->localizedModuleName(QLocale::Russian) : "",
                                          var->baseType.name
            );
            result = constants_[indx].value.toInt();
        }

        if (!bounds.second->variable) {
            result = abs(result - bounds.second->constant.toInt());
        }
        else{
            AST::VariablePtr var = bounds.second->variable;
            indx = calculateConstantValue(parseValueType(var->baseType), var->dimension, var->initialValue,
                                          var->baseType.actor ? var->baseType.actor->localizedModuleName(QLocale::Russian) : "",
                                          var->baseType.name
            );

            result = abs(result - constants_[indx].value.toInt());
        }

        return  result;
    }
}
#endif //KUMIR2_VARINITIALIZATIONGENERATOR_H
