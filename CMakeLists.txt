cmake_minimum_required(VERSION 3.20)
project(kwisatz CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(kwisatz
    src/main.cc
    src/lex/lexer.cc
    src/lex/token.cc
    src/parse/parser.cc
)
