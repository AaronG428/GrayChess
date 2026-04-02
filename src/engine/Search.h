#pragma once
#include "../board/Board.h"
#include "../move/MoveList.h"
#include "TranspositionTable.h"

class Search {
public:
    static constexpr int INF      = 1'000'000;
    static constexpr int MATE_VAL = 900'000;

    explicit Search(TranspositionTable& tt);

    // Iterative deepening — returns best move found at maxDepth.
    Move findBestMove(Board& board, int maxDepth);

    // Exposed for unit testing.
    int negamax(Board& board, int depth, int alpha, int beta, int ply = 0);
    int quiesce(Board& board, int alpha, int beta);

private:
    TranspositionTable& tt_;

    void orderMoves(MoveList& moves, int ttBestFrom, int ttBestTo) const;
    int  mvvLva(const Move& m) const;
};
