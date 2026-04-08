#pragma once
#include "../board/Board.h"
#include "../move/MoveList.h"
#include "TranspositionTable.h"
#include <atomic>
#include <chrono>
#include <functional>
#include <unordered_map>

using namespace std;

class Search {
public:
    static constexpr int INF      = 1'000'000;
    static constexpr int MATE_VAL = 900'000;

    explicit Search(TranspositionTable& tt);

    // Called after each completed depth: (depth, score, bestMove).
    using InfoCallback = std::function<void(int, int, const Move&)>;

    // Iterative deepening — returns best move found at maxDepth.
    // If cb is provided it is called after each completed depth.
    Move findBestMove(Board& board, int maxDepth, InfoCallback cb = nullptr);

    // Exposed for unit testing.
    int negamax(Board& board, int depth, int alpha, int beta, int ply = 0);
    int quiesce(Board& board, int alpha, int beta);

    // Time / stop control — called from UCI.
    void stop()      { stop_ = true; }
    void resetStop() { stop_ = false; nodes_ = 0;
                       searchStart_ = std::chrono::steady_clock::now();
                       deadline_    = std::chrono::steady_clock::time_point::max(); }
    void setTimeLimit(long ms) {
        deadline_ = std::chrono::steady_clock::now()
                  + std::chrono::milliseconds(ms);
    }

    // Search statistics — read from UCI info callback.
    uint64_t nodes() const { return nodes_; }
    long elapsedMs() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now() - searchStart_).count();
    }

    // Exposed for unit testing (would otherwise be private).
    void orderMoves(MoveList& moves, int ttBestFrom, int ttBestTo) const;
    int  moveRankValue(const Move& m, int ttBestFrom, int ttBestTo) const;
    int  hashMove(const Move& m, int ttBestFrom, int ttBestTo) const;
    int  mvvLva(const Move& m) const;

private:
    TranspositionTable& tt_;
    unordered_map<Move::MoveEnum, int> moveTypeMap;

    std::atomic<bool> stop_{false};
    std::chrono::steady_clock::time_point searchStart_{ std::chrono::steady_clock::now() };
    std::chrono::steady_clock::time_point deadline_{ std::chrono::steady_clock::time_point::max() };
    uint64_t nodes_ = 0;
        
};
