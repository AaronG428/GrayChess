#pragma once
#include "MoveList.h"

class Board; // forward declaration

class MoveGenerator {
public:
    // Generate all pseudo-legal moves for the side to move.
    static MoveList generateMoves(const Board& board);

    // Generate only legal moves (filters out moves that leave the king in check).
    // Takes Board& because it uses applyMove/unmakeMove internally (no copy).
    static MoveList generateLegalMoves(Board& board);

    // Generate captures only — used by quiescence search in Phase 9.
    static MoveList generateCaptures(const Board& board);

    static MoveList generateLegalCaptures(Board& board);

    static void MVVLVA(MoveList& movelist);
};
