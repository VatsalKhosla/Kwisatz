#include <cstdio>
#include <string>

#include "kwisatz/util/version.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "kwisatz %s\n", kwisatz::kVersion);
        std::fprintf(stderr, "usage: %s <source.kw>\n", argv[0]);
        return 1;
    }

    std::string input_path = argv[1];
    std::printf("compiling %s\n", input_path.c_str());
    
    return 0;
}
