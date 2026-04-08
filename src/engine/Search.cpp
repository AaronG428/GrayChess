#include "Search.h"
#include "../move/MoveGenerator.h"
#include "Eval.h"
#include <iostream>

Search::Search(TranspositionTable& tt) : tt_(tt) {
    moveTypeMap = {
        {Move::PromoteCapture, 5},
        {Move::Capture,        4},
        {Move::EnPassant,      3},
        {Move::Promote,        2},
        {Move::Castle,         1},
        {Move::Quiet,          0}}; 

    // std:://cout << "moveTypeMap: " << moveTypeMap.at(Move::PromoteCapture) << std::endl;
}

static constexpr int INF      = 1'000'000;
static constexpr int MATE_VAL = 900'000;

Move Search::findBestMove(Board& board, int maxDepth) {
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
    // std:://cout << "depth: " << depth << std::endl;
    uint64_t hash = board.hash;
    TTEntry* entry = tt_.probe(hash);
    int ttBestFrom = -1, ttBestTo = -1;
    Move bestMove;

    // If already searched this position at greater depth
    if ((entry != nullptr) &&  (entry->depth >= depth)){

        // true minimax value, always usable
        if (entry->type == NodeType::Exact){
            return entry->score;
        }

        // A move here caused a beta cutoff in a previous search.
        // The true value is >= entry.score, so we can raise alpha.
        if (entry->type == NodeType::LowerBound){
            alpha = max(alpha, entry->score);
        }
        
        // Every move here failed low in a previous search.
        // The true value is <= entry.score, so we can lower beta.
        if (entry->type == NodeType::UpperBound){
            beta = min(beta, entry->score);
        }

        ttBestFrom = entry->bestMove.from;
        ttBestTo = entry->bestMove.to;
        bestMove = (entry->bestMove);

    }

    MoveList moves = MoveGenerator::generateLegalMoves(board);


    if (moves.count == 0){
        //Checkmate
        //Add ply so shallower mates are better for opponent
        if(board.check()){
            return -(MATE_VAL-ply);
        }
        //Stalemate
        return 0;
    }

    if(depth == 0){
        return quiesce(board, alpha, beta);
    }

    
    // test code
    // for(int i=0; i<moves.count; i++){
    //     Move m = moves.moves[i];
    //     // std:://cout << "moveType:" << m.moveType << std::endl;
    //     // std:://cout << "move to:" << m.to << std::endl;
    //     // std:://cout << "move from:" << m.from << std::endl;
    //     // std:://cout << "move piece:" << m.piece << std::endl;
    // }
    // std:://cout << "-----------End move list----------" << std::endl;

    // end test code
    // If no legal moves

    // std:://cout << "moves length: " << moves.count << std::endl;
    orderMoves(moves, ttBestFrom, ttBestTo);
    // std:://cout << "ordered moves:" << moves.count << std::endl;
    bool updatedAlpha = false;
    // Move* bestMove  
    for(int i=0; i<moves.count; i++){
        Move move = moves.moves[i];
        board.applyMove(move);
        // std:://cout << "pre negamax" << std::endl;
        int score = -negamax(board, depth-1, -beta, -alpha, ply+1);
        // std:://cout << "post negamax" << std::endl;
        board.unmakeMove();

        if (score >= beta){
            tt_.store(hash, beta, depth, NodeType::LowerBound, move);
            return beta;
        } 
        if (score > alpha){
            alpha = score;
            bestMove = move;
            updatedAlpha = true;
        }
    }
    if (updatedAlpha){
        tt_.store(hash, alpha, depth, NodeType::Exact, bestMove);

    }else{
        tt_.store(hash, alpha, depth, NodeType::UpperBound, moves.moves[0]);
    }
    
    return alpha;
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
    // test code
    // for (Move m:captures.moves){
    //     // std:://cout << "moveType:" << m.moveType << std::endl;
    //     // std:://cout << "move to:" << m.to << std::endl;
    //     // std:://cout << "move from:" << m.from << std::endl;
    //     // std:://cout << "move piece:" << m.piece << std::endl;
    // }
    
    // end test code
    if(captures.count > 0){
        orderMoves(captures, -1, -1);

        for (int i=0; i<captures.count; i++){
            Move capture = captures.moves[i];
            board.applyMove(capture);
            int score = -quiesce(board, -beta, -alpha);
            board.unmakeMove();
            if (score >= beta) return beta;
            if (score > alpha) alpha = score;
        }
    }
    return alpha;
}

void Search::orderMoves(MoveList& moves, int ttBestFrom, int ttBestTo) const {
    std::sort(moves.moves, moves.moves+moves.count, [=](Move a, Move b){
        return moveRankValue(a, ttBestFrom, ttBestTo) > moveRankValue(b, ttBestFrom, ttBestTo); //< because sort descending
    });
}

int Search::moveRankValue(const Move&m, int ttBestFrom, int ttBestTo) const{
    // // std:://cout << "moveType:" << m.moveType << std::endl;
    // // std:://cout << "move to:" << m.to << std::endl;
    // // std:://cout << "move from:" << m.from << std::endl;
    // // std:://cout << "move piece:" << m.piece << std::endl;
    // // std:://cout << "move rank value calc" << std::endl;
    //cout << "Ranking " << endl;
    int value = hashMove(m, ttBestFrom, ttBestTo)*100000;
    // std:://cout << "hash value calced" << std::endl;
    //cout << "value: " << value << endl;
    value += mvvLva(m);
    //cout << "value: " << value << endl;
    // std:://cout << "mvvLva calced" << std::endl;
    
    value += moveTypeMap.at(m.moveType); //values declared in Search.h
    //cout << "value: " << value << endl;
    return value;
}

//integer value of 1 if m is the principal move (best hashed moved)
int Search::hashMove(const Move& m, int ttBestFrom, int ttBestTo) const{
    return (m.from == ttBestFrom)&(m.to == ttBestTo); 
}

//most significant factor higher sig digit
int Search::mvvLva(const Move& m) const {
    if((m.moveType == Move::Capture)||(m.moveType == Move::PromoteCapture)||(m.moveType == Move::EnPassant)){

        return 10*MATERIAL[m.cPiece] - MATERIAL[m.piece]; 
    }
    return 0;
}


