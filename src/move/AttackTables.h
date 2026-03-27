#pragma once
#include <cstdint>
#include "../utils/Bits.h"

// Pre-computed attack tables.  Call AttackTables::init() once at program start.
//
// Board orientation: h1=bit0, a1=bit7, h8=bit56, a8=bit63
//   sq = (rank-1)*8 + (7-file_from_a)   where file_from_a: a=0 .. h=7
//
// Pawn color index: 0=white, 1=black

namespace AttackTables {

// ---------------------------------------------------------------------------
// Trivial tables (no occupancy dependence)
// ---------------------------------------------------------------------------
extern uint64_t KNIGHT_ATTACKS[64];
extern uint64_t KING_ATTACKS[64];
extern uint64_t PAWN_ATTACKS[2][64]; // [color][square]

// ---------------------------------------------------------------------------
// Magic bitboard entry for sliding pieces
// ---------------------------------------------------------------------------
struct MagicEntry {
    uint64_t  mask;   // relevant occupancy bits for this square
    uint64_t  magic;  // multiplier found by trial-and-error at init time
    int       shift;  // right-shift amount = 64 - popcount(mask)
    uint64_t* table;  // pointer into the flat attack storage array
};

extern MagicEntry ROOK_MAGICS[64];
extern MagicEntry BISHOP_MAGICS[64];

// ---------------------------------------------------------------------------
// O(1) attack lookups — call these in the hot path
// ---------------------------------------------------------------------------
inline uint64_t rookAttacks(int sq, uint64_t occ) {
    const MagicEntry& m = ROOK_MAGICS[sq];
    return m.table[((occ & m.mask) * m.magic) >> m.shift];
}

inline uint64_t bishopAttacks(int sq, uint64_t occ) {
    const MagicEntry& m = BISHOP_MAGICS[sq];
    return m.table[((occ & m.mask) * m.magic) >> m.shift];
}

inline uint64_t queenAttacks(int sq, uint64_t occ) {
    return rookAttacks(sq, occ) | bishopAttacks(sq, occ);
}

// ---------------------------------------------------------------------------
// Initialisation — populates all tables (must be called before any lookup)
// ---------------------------------------------------------------------------
void init();

} // namespace AttackTables
