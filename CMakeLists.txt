cmake_minimum_required(VERSION 3.15)
project(GrayChess CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-O3 -march=native -Wall -Wextra)

add_library(engine
    src/move/Move.cpp
    src/board/Board.cpp
    src/move/MoveGenerator.cpp
    src/move/AttackTables.cpp
)
target_include_directories(engine PUBLIC src/)

add_executable(BoardTests tests/BoardTests.cpp)
target_link_libraries(BoardTests engine)
