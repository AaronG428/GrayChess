#pragma once
#include "../board/Board.h"
#include "../move/MoveList.h"
#include "TranspositionTable.h"
#include <atomic>
#include <chrono>
#include <functional>
#include <vector>

using namespace std;

class Search {
public:
    static constexpr int INF      = 1'000'000;
    static constexpr int MATE_VAL = 900'000;

    explicit Search(TranspositionTable& tt);

    // Called after each completed depth:
    //   (multipv index, depth, score, pv line as move list)
    using InfoCallback = std::function<void(int, int, int, const std::vector<Move>&)>;

    // Iterative deepening — returns best move found at maxDepth.
    // If cb is provided it is called after each completed depth for each PV.
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
    void setMultiPV(int n) { multiPV_ = std::max(1, n); }
    void setGameHistory(const std::vector<uint64_t>& h) { hashHistory_ = h; }

    // Null move pruning reduction depth (R). Set to 0 to disable.
    // Exposed so tests can toggle it on/off without rebuilding.
    void setNullMoveR(int r) { nullMoveR_ = r; }

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
    // Move-type bonus indexed by Move::MoveEnum (Quiet=0,Castle=1,Capture=2,Promote=3,PromoteCapture=4,EnPassant=5)
    static constexpr int MOVE_TYPE_SCORE[6] = {0, 1, 4, 2, 5, 3};
    int multiPV_  = 1;
    int nullMoveR_ = 3;

    std::atomic<bool> stop_{false};
    std::chrono::steady_clock::time_point searchStart_{ std::chrono::steady_clock::now() };
    std::chrono::steady_clock::time_point deadline_{ std::chrono::steady_clock::time_point::max() };
    uint64_t nodes_ = 0;
    std::vector<uint64_t> hashHistory_;

    // Root-level search with a list of (from,to) moves to exclude.
    // Used to extract additional PV lines without re-searching the first.
    int rootSearch(Board& board, int depth,
                   const std::vector<std::pair<int,int>>& excluded);

    // Walk the TT from the current position to extract the best line.
    std::vector<Move> extractPV(Board board, int maxDepth) const;
};
