#ifndef ARDUINO_DATA_H
#define ARDUINO_DATA_H

#include "arduino_tableelem.hpp"

#include <deque>
#include <stdint.h>
#include "arduino_enums.hpp"
#include <QDebug>


namespace Arduino {
using Arduino::ElemType;

struct Data {
    std::deque <TableElem> d;
    uint8_t versionMaj;
    uint8_t versionMin;
    uint8_t versionRel;
    unsigned long lastModified;
};

inline void makeHelpersForTextRepresentation(const Data & data, AS_Helpers & helpers)
{
    Kumir::EncodingError encodingError;
    for (size_t i=0; i<data.d.size(); i++) {
        const TableElem & e = data.d.at(i);
        if (e.type==Arduino::EL_LOCAL) {
            AS_Key akey((e.module<<16)|e.algId, e.id);
            if (helpers.locals.count(akey)==0) {
                const std::string value = Kumir::Coder::encode(Kumir::UTF8, e.name, encodingError);
                helpers.locals.insert(std::pair<AS_Key,std::string>(akey,value));
            }
        }
        if (e.type==Arduino::EL_GLOBAL || e.type==Arduino::EL_EXTERN || e.type==Arduino::EL_FUNCTION || e.type==Arduino::EL_MAIN ) {
            AS_Key akey(e.module<<16, e.type==Arduino::EL_GLOBAL? e.id : e.algId);
            AS_Values * vals = e.type==Arduino::EL_GLOBAL? &(helpers.globals) : &(helpers.algorithms);
            if (vals->count(akey)==0) {
                const std::string value = Kumir::Coder::encode(Kumir::UTF8, e.name, encodingError);
                vals->insert(std::pair<AS_Key,std::string>(akey,value));
            }
        }
        if (e.type==Arduino::EL_CONST) {
            AS_Key akey(0, e.id);
            if (helpers.constants.count(akey)==0) {
                const std::string value = Kumir::Coder::encode(Kumir::UTF8, e.initialValue.toString(), encodingError);
                helpers.constants.insert(std::pair<AS_Key,std::string>(akey,value));
            }
        }
    }
}

inline void bytecodeToTextStream(std::ostringstream & ts, const Data & data)
{
    ts << "// license\n";
    ts << "// generated from kumir version " << int(data.versionMaj) << " " << int(data.versionMin) << " " << int(data.versionRel) << "\n\n";
    AS_Helpers helpers;
    for (size_t i=0; i<data.d.size(); i++) {
        tableElemToTextStream(ts, data.d.at(i), helpers);
        makeHelpersForTextRepresentation(data, helpers);
        qCritical() << ts.str().c_str();
    }
}


} // namespace Arduino

#endif // ARDUINO_DATA_H
