#pragma once
#include <vector>
#include "../move/Move.h"

class Board; // forward declaration

class MoveGenerator {
public:
    static std::vector<Move> generateMoves(Board& board);
};
