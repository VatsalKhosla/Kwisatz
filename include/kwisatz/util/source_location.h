#pragma once

#include <string> 

namespace kwisatz {

struct SourceLocation{
    std::string file;
    int line;
    int column;
};

}