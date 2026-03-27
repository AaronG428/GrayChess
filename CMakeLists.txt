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

add_executable(TestBits tests/TestBits.cpp)
target_link_libraries(TestBits engine)

add_executable(TestAttackTables tests/TestAttackTables.cpp)
target_link_libraries(TestAttackTables engine)

add_executable(TestBoard tests/TestBoard.cpp)
target_link_libraries(TestBoard engine)

add_executable(TestMoveGen tests/TestMoveGen.cpp)
target_link_libraries(TestMoveGen engine)

enable_testing()
add_test(NAME Bits         COMMAND TestBits)
add_test(NAME AttackTables COMMAND TestAttackTables)
add_test(NAME Board        COMMAND TestBoard)
add_test(NAME MoveGen      COMMAND TestMoveGen)
