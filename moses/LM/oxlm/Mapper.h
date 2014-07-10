#pragma once

#include <map>
#include "corpus/corpus.h"
#include "moses/Factor.h"

namespace Moses
{
class OXLMMapper
{
public:
 OXLMMapper(const oxlm::Dict& dict);

 int convert(const Moses::Factor *factor) const;

private:
 void add(int lbl_id, int cdec_id);

 oxlm::Dict dict;
 typedef std::map<const Moses::Factor*, int> Coll;
 Coll moses2lbl;
 int kUNKNOWN;

};

}
