#include "Search.h"
#include "../move/MoveGenerator.h"
#include "Eval.h"

Search::Search(TranspositionTable& tt) : tt_(tt) {}

static constexpr int INF      = 1'000'000;
static constexpr int MATE_VAL = 900'000;

Move Search::findBestMove(Board& board, int maxDepth) {
    Move best;
    for (int depth = 1; depth <= maxDepth; depth++) {
        negamax(board, depth, -INF, INF);
        // best move is the TT entry for the root position after each iteration
    }
    return tt_.probe(board.hash)->bestMove;
    
}


    //  1. probe TT — if hit and depth >= remaining, return/adjust bounds from entry
    //  2. if depth == 0, return quiesce(board, alpha, beta)
    //  3. generate legal moves; if none → checkmate (-INF + ply) or stalemate (0)
    //  4. order moves (TT best move first, then MVV-LVA for captures, then quiets)
    //  5. for each move:
    //       applyMove; score = -negamax(board, depth-1, -beta, -alpha); unmakeMove
    //       if score >= beta  → store LowerBound in TT, return beta  (cutoff)
    //       if score > alpha  → alpha = score; bestMove = move
    //  6. store Exact (or UpperBound if no improvement) in TT
    //  7. return alpha



    //  Mate score encoding: return -(MATE_VAL - ply) so shorter mates score higher. The ply parameter threads through recursion
    //  (depth from root, not remaining depth).
int Search::negamax(Board& board, int depth, int alpha, int beta, int ply) {
    // TODO Phase 9: TT probe, terminal detection, move ordering, recursive search
    (void)board; (void)depth; (void)alpha; (void)beta; (void)ply;
    uint64_t hash = board.hash;
    TTEntry* position = tt_.probe(hash);
    // if (position != nullptr){
    //     if (position->depth )
    // }
    return 0;
}


    //  1. stand-pat = evaluate(board)
    //  2. if stand-pat >= beta  → return beta      (cutoff)
    //  3. if stand-pat > alpha  → alpha = stand-pat
    //  4. generate captures only (MoveGenerator::generateCaptures)
    //  5. order captures by MVV-LVA
    //  6. for each capture:
    //       applyMove; score = -quiesce(board, -beta, -alpha); unmakeMove
    //       if score >= beta  → return beta
    //       if score > alpha  → alpha = score
    //  7. return alpha

    //  No TT probing in quiescence (complexity vs. benefit not worth it at this stage).
int Search::quiesce(Board& board, int alpha, int beta) {
    // TODO Phase 9: stand-pat, generate captures, recurse
    int standPat = evaluate(board);
    if (standPat >= beta) return beta;
    if (standPat > alpha) alpha = standPat;

    MoveList captures = MoveGenerator::generateLegalCaptures(board);
    MoveGenerator::MVVLVA(captures);
    for (Move capture : captures.moves){
        board.applyMove(capture);
        int score = -quiesce(board, -beta, -alpha);
        board.unmakeMove();
        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }
    return alpha;
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
