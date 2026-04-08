cmake_minimum_required(VERSION 3.15)
project(GrayChess CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -O0)
else()
    add_compile_options(-O3 -march=native)
endif()

add_library(engine
    src/move/Move.cpp
    src/board/Board.cpp
    src/move/MoveGenerator.cpp
    src/move/AttackTables.cpp
    src/engine/Eval.cpp
    src/engine/Zobrist.cpp
    src/engine/TranspositionTable.cpp
    src/engine/Search.cpp
)
target_include_directories(engine PUBLIC src/)

add_executable(BoardTests tests/BoardTests.cpp)
target_link_libraries(BoardTests engine)

add_executable(TestBits tests/TestBits.cpp)
target_link_libraries(TestBits engine)

add_executable(TestAttackTables tests/TestAttackTables.cpp)
target_link_libraries(TestAttackTables engine)

add_executable(TestBoard tests/TestBoard.cpp)
target_link_libraries(TestBoard engine)

add_executable(TestMoveGen tests/TestMoveGen.cpp)
target_link_libraries(TestMoveGen engine)

add_executable(PerftTests tests/PerftTests.cpp)
target_link_libraries(PerftTests engine)

add_executable(TestEval tests/TestEval.cpp)
target_link_libraries(TestEval engine)

add_executable(TestZobrist tests/TestZobrist.cpp)
target_link_libraries(TestZobrist engine)

add_executable(TestTranspositionTable tests/TestTranspositionTable.cpp)
target_link_libraries(TestTranspositionTable engine)

enable_testing()
add_test(NAME Bits                COMMAND TestBits)
add_test(NAME AttackTables        COMMAND TestAttackTables)
add_test(NAME Board               COMMAND TestBoard)
add_test(NAME MoveGen             COMMAND TestMoveGen)
add_test(NAME Perft               COMMAND PerftTests)
add_test(NAME Eval                COMMAND TestEval)
add_test(NAME Zobrist             COMMAND TestZobrist)
add_test(NAME TranspositionTable  COMMAND TestTranspositionTable)

add_executable(TestSearch tests/TestSearch.cpp)
target_link_libraries(TestSearch engine)
add_test(NAME Search COMMAND TestSearch)

add_executable(GrayChess src/main.cpp src/uci/UCI.cpp)
target_link_libraries(GrayChess engine)

add_executable(TestUCI tests/TestUCI.cpp src/uci/UCI.cpp)
target_link_libraries(TestUCI engine)
add_test(NAME UCI COMMAND TestUCI)
