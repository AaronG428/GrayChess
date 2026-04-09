#include "Eval.h"
#include "../utils/Bits.h"

//pawn, Bishop, kNight, Rook, Queen, King
//default values before experimentation
const int MATERIAL[6] = {100, 330, 320, 500, 900, 20000};

// PSTs indexed by square (h1=0, a8=63).
// Board uses: file = 7-(sq%8), rank = sq/8 (0=rank1).
// For black, mirror vertically: sq ^ 56.
//
// Values are intentionally small (±10-20 cp) — just enough to break symmetry
// and guide the search without encoding strong opening opinions.
//
// Layout: sq=0 is h1 (bottom-right), sq=63 is a8 (top-left).
// Rows below are printed rank8→rank1 (top to bottom) for readability,
// columns are a→h (left to right), so the array is stored in reverse-rank order.

// clang-format off

// Pawns: reward central advance, penalise edges
static constexpr int PST_PAWN[64] = {
//  a    b    c    d    e    f    g    h
    0,   0,   0,   0,   0,   0,   0,   0,  // rank 8 (should never have pawns)
   10,  10,  10,  10,  10,  10,  10,  10,  // rank 7 (one step from promotion)
    5,   5,   8,  12,  12,   8,   5,   5,  // rank 6
    2,   2,   5,  10,  10,   5,   2,   2,  // rank 5
    0,   0,   2,   8,   8,   2,   0,   0,  // rank 4
    0,  -2,  -2,   2,   2,  -2,  -2,   0,  // rank 3
    0,   2,   2,  -8,  -8,   2,   2,   0,  // rank 2 (starting rank)
    0,   0,   0,   0,   0,   0,   0,   0,  // rank 1 (should never have pawns)
};

// Knights: strongly prefer the centre, hate the rim
static constexpr int PST_KNIGHT[64] = {
// a    b    c    d    e    f    g    h
  -20, -10, -10, -10, -10, -10, -10, -20, // rank 8
  -10,  -5,   0,   0,   0,   0,  -5, -10, // rank 7
  -10,   0,  10,  12,  12,  10,   0, -10, // rank 6
  -10,   5,  12,  15,  15,  12,   5, -10, // rank 5
  -10,   0,  12,  15,  15,  12,   0, -10, // rank 4
  -10,   5,  10,  12,  12,  10,   5, -10, // rank 3
  -10,  -5,   0,   5,   5,   0,  -5, -10, // rank 2
  -20, -10, -10, -10, -10, -10, -10, -20, // rank 1
};

// Bishops: prefer long diagonals and centre
static constexpr int PST_BISHOP[64] = {
// a    b    c    d    e    f    g    h
   -5,  -5,  -5,  -5,  -5,  -5,  -5,  -5, // rank 8
   -5,   5,   0,   0,   0,   0,   5,  -5, // rank 7
   -5,   0,   8,   5,   5,   8,   0,  -5, // rank 6
   -5,   5,   5,  10,  10,   5,   5,  -5, // rank 5
   -5,   0,  10,  10,  10,  10,   0,  -5, // rank 4
   -5,  10,  10,  10,  10,  10,  10,  -5, // rank 3
   -5,   5,   0,   0,   0,   0,   5,  -5, // rank 2
   -5,  -5,  -5,  -5,  -5,  -5,  -5,  -5, // rank 1
};

// Rooks: reward open files and 7th rank
static constexpr int PST_ROOK[64] = {
// a    b    c    d    e    f    g    h
    5,   5,   5,   5,   5,   5,   5,   5, // rank 8
   10,  10,  10,  10,  10,  10,  10,  10, // rank 7 (strong on 7th)
    0,   0,   0,   0,   0,   0,   0,   0, // rank 6
    0,   0,   0,   0,   0,   0,   0,   0, // rank 5
    0,   0,   0,   0,   0,   0,   0,   0, // rank 4
    0,   0,   0,   0,   0,   0,   0,   0, // rank 3
   -5,  -5,  -5,  -5,  -5,  -5,  -5,  -5, // rank 2
    0,   0,   0,   5,   5,   0,   0,   0, // rank 1 (connect rooks / castle)
};

// Queens: avoid early development, prefer centre in middlegame
static constexpr int PST_QUEEN[64] = {
// a    b    c    d    e    f    g    h
  -10,  -5,  -5,  -5,  -5,  -5,  -5, -10, // rank 8
   -5,   0,   0,   0,   0,   0,   0,  -5, // rank 7
   -5,   0,   5,   5,   5,   5,   0,  -5, // rank 6
   -5,   0,   5,   8,   8,   5,   0,  -5, // rank 5
   -5,   0,   5,   8,   8,   5,   0,  -5, // rank 4
   -5,   0,   5,   5,   5,   5,   0,  -5, // rank 3
   -5,   0,   0,   0,   0,   0,   0,  -5, // rank 2
  -10,  -5,  -5,  -5,  -5,  -5,  -5, -10, // rank 1
};

// King: stay safe behind pawns in opening/middlegame
static constexpr int PST_KING[64] = {
// a    b    c    d    e    f    g    h
  -30, -30, -30, -30, -30, -30, -30, -30, // rank 8
  -30, -30, -30, -30, -30, -30, -30, -30, // rank 7
  -30, -30, -30, -30, -30, -30, -30, -30, // rank 6
  -30, -30, -30, -30, -30, -30, -30, -30, // rank 5
  -20, -20, -20, -20, -20, -20, -20, -20, // rank 4
  -10, -10, -10, -10, -10, -10, -10, -10, // rank 3
    5,   5,  -5, -10, -10,  -5,  10,   5, // rank 2 (castled kings are OK)
   10,  15,   0,   0,   0,   0,  15,  10, // rank 1 (corner = safer)
};

// clang-format on

static constexpr const int* PST[6] = {
    PST_PAWN, PST_BISHOP, PST_KNIGHT, PST_ROOK, PST_QUEEN, PST_KING
};

// The PST arrays are laid out rank8→rank1, a→h.
// sq uses h1=0: file_from_h = sq%8, rank = sq/8 (0=rank1).
// To index PST: row = 7-rank (flip to rank8-first), col = 7-file_from_h (flip to a-first).
static inline int pstScore(int pieceType, int sq) {
    int rank = sq / 8;
    int file = sq % 8;          // 0=h, 7=a
    int row  = 7 - rank;        // rank8=0, rank1=7
    int col  = 7 - file;        // a=0, h=7
    return PST[pieceType][row * 8 + col];
}

static int piecePST(uint64_t bb, int pieceType, bool isBlack) {
    int score = 0;
    while (bb) {
        int sq = lsb(bb);
        bb &= bb - 1;
        int effectiveSq = isBlack ? (sq ^ 56) : sq;  // mirror rank for black
        score += pstScore(pieceType, effectiveSq);
    }
    return score;
}

int evaluate(const Board& board){
    int score = 0;
    for (int i = 0; i < 6; i++) {
        uint64_t wBB = board.pieceBB[i + 1];
        uint64_t bBB = board.pieceBB[i + 8];
        score += MATERIAL[i] * popcount(wBB);
        score -= MATERIAL[i] * popcount(bBB);
        score += piecePST(wBB, i, false);
        score -= piecePST(bBB, i, true);
    }
    return board.whiteTurn ? score : -score;
}