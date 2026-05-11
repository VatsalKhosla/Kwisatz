#pragma once

#include<ostream>

#include"kwisatz/ast/ast.h"
#include"kwisatz/ir/translate.h"

namespace kwisatz{

void printIrProgram(const Program& prog,const IrTranslator& tr,std::ostream& out);

}
