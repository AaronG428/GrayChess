#include "Eval.h"
#include "../utils/Bits.h"

//pawn, Bishop, kNight, Rook, Queen, King
//default values before experimentation
const int MATERIAL[6] = {100, 330, 320, 500, 900, 20000};

//ignore PST for now
//TODO: implement PST
int evaluate(const Board& board){
    int score = 0;
    for (int i=0; i<6; i++) {
        int pieceIdx = i+1;
        score += MATERIAL[i]*popcount(board.pieceBB[pieceIdx]);
    }
    for (int i=0; i<6; i++) {
        int pieceIdx = i+8;
        score -= MATERIAL[i]*popcount(board.pieceBB[pieceIdx]);
    }
    return board.whiteTurn ? score : -score;
}