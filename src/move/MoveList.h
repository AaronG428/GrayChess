#pragma once
#include "Move.h"

// Stack-allocated move list — avoids heap allocation in the search tree.
// 256 entries is a safe upper bound for any legal chess position.
struct MoveList {
    static constexpr int MAX_MOVES = 256;
    Move moves[MAX_MOVES];
    int  count = 0;

    void push(const Move& m) { moves[count++] = m; }
};
