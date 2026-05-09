#pragma once

#include<ostream>

#include"kwisatz/ast/ast.h"

namespace kwisatz{

void printProgram(const Program& prog,std::ostream& out);

}
