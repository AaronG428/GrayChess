#include "MoveGenerator.h"
#include "../board/Board.h"

std::vector<Move> MoveGenerator::generateMoves(Board& board) {
    if (board.checkMate() || board.staleMate()) {
        return {};
    }

    // TODO Phase 4: generate pseudo-legal moves for all piece types
    // TODO Phase 5: filter for legality (don't leave king in check)

    return {};
}
