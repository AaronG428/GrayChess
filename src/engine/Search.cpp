#include "Search.h"
#include "../move/MoveGenerator.h"

Search::Search(TranspositionTable& tt) : tt_(tt) {}

Move Search::findBestMove(Board& board, int maxDepth) {
    // TODO Phase 9: iterative deepening loop (depth 1..maxDepth)
    (void)board; (void)maxDepth;
    return Move{};
}

int Search::negamax(Board& board, int depth, int alpha, int beta, int ply) {
    // TODO Phase 9: TT probe, terminal detection, move ordering, recursive search
    (void)board; (void)depth; (void)alpha; (void)beta; (void)ply;
    return 0;
}

int Search::quiesce(Board& board, int alpha, int beta) {
    // TODO Phase 9: stand-pat, generate captures, recurse
    (void)board; (void)alpha; (void)beta;
    
    return 0;
}

void Search::orderMoves(MoveList& moves, int ttBestFrom, int ttBestTo) const {
    // TODO Phase 9: score each move (TT best, MVV-LVA, quiets), sort descending
    (void)moves; (void)ttBestFrom; (void)ttBestTo;
}

int Search::mvvLva(const Move& m) const {
    // TODO Phase 9: 10 * MATERIAL[victim] - MATERIAL[attacker]
    (void)m;
    return 0;
}
