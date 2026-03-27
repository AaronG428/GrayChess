#pragma once
#include "MoveList.h"

class Board; // forward declaration

class MoveGenerator {
public:
    // Generate all pseudo-legal moves for the side to move.
    // Phase 5 will wrap this with legality filtering.
    static MoveList generateMoves(const Board& board);

    // Generate captures only — used by quiescence search in Phase 9.
    static MoveList generateCaptures(const Board& board);
};
