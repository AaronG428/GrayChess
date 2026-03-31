#include "Zobrist.h"
#include "../board/Board.h"
#include <random>

namespace Zobrist {

uint64_t PIECE_KEYS[12][64];
uint64_t SIDE_KEY;
uint64_t CASTLE_KEYS[16];
uint64_t EP_KEYS[8];

void init() {
    std::mt19937_64 rng(0x6769FACEDEADBEEFULL); // fixed seed for reproducibility
    for(int i=0; i<12; i++){
        for(int j=0; j<64; j++){
            PIECE_KEYS[i][j] = rng();
        }
    }
    SIDE_KEY = rng();
    for(int i=0; i<16; i++){
        CASTLE_KEYS[i] = rng();
    }
    for(int i=0; i<8; i++){
        EP_KEYS[i] = rng();
    }
}

uint64_t compute(const Board& board) {
    // TODO Phase 8: XOR all occupied squares, side, castle rights, ep file
    uint64_t hash = 0;

    // White pieces
    for(int i=0; i<6; i++){
        int piece_index = i+1;
        uint64_t piece_bb = board.pieceBB[piece_index];
        for(int sq=0; sq<64; sq++){
            if(piece_bb & (1ULL << sq) > 0){
                hash ^= PIECE_KEYS[i][sq];
            }
        }
    }
    // Black pieces
    for(int i=6; i<12; i++){
        int piece_index = i+2;
        uint64_t piece_bb = board.pieceBB[piece_index];
        for(int sq=0; sq<64; sq++){
            if(piece_bb & (1ULL << sq) > 0){
                hash ^= PIECE_KEYS[i][sq];
            }
        }
    }
    // Side
    if (board.whiteTurn) hash ^= SIDE_KEY;
    // Castle
    int rights = (board.castleRights[0] << 0)                                                                                         
             | (board.castleRights[1] << 1)                                                                          
             | (board.castleRights[2] << 2)                                                                                         
             | (board.castleRights[3] << 3);  
    
    hash ^= CASTLE_KEYS[rights];

    //EP File
    



    return hash;
}

} // namespace Zobrist
