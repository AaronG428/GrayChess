#include "AttackTables.h"
#include "Move.h"   // for the slow propagators used during table generation only
#include "../utils/Bits.h"
#include <cstring>
#include <random>

namespace AttackTables {

// ---------------------------------------------------------------------------
// Storage
// ---------------------------------------------------------------------------
uint64_t KNIGHT_ATTACKS[64];
uint64_t KING_ATTACKS[64];
uint64_t PAWN_ATTACKS[2][64];

MagicEntry ROOK_MAGICS[64];
MagicEntry BISHOP_MAGICS[64];

// Flat attack tables — MagicEntry.table points into these.
// Rook: max 12-bit mask (corner) → 2^12 = 4096 entries per square
// Bishop: max 9-bit mask (centre) → 2^9  = 512  entries per square
static uint64_t rookAttackStorage  [64][4096];
static uint64_t bishopAttackStorage[64][512];

// ---------------------------------------------------------------------------
// Mask generation (h1=bit0 convention)
//
// The mask for a slider on sq contains only the interior blocking squares:
//   Rook:   same rank (files 1-6) + same file (ranks 1-6), minus own square
//   Bishop: all diagonal squares that are not on the board edge (ranks/files 1-6)
//
// Edge squares are excluded because their occupancy never affects whether
// intermediate squares are reachable — the slider either reaches the edge or
// is blocked before it, regardless of what is ON the edge.
// ---------------------------------------------------------------------------
static uint64_t rookMask(int sq) {
    uint64_t mask = 0;
    int rank = sq / 8, file = sq % 8;
    for (int f = 1; f <= 6; f++) if (f != file) mask |= 1ULL << (rank*8 + f);
    for (int r = 1; r <= 6; r++) if (r != rank) mask |= 1ULL << (r*8 + file);
    return mask;
}

static uint64_t bishopMask(int sq) {
    uint64_t mask = 0;
    int r0 = sq / 8, f0 = sq % 8;
    for (int r=r0+1, f=f0+1; r<=6 && f<=6; r++, f++) mask |= 1ULL << (r*8+f);
    for (int r=r0+1, f=f0-1; r<=6 && f>=1; r++, f--) mask |= 1ULL << (r*8+f);
    for (int r=r0-1, f=f0+1; r>=1 && f<=6; r--, f++) mask |= 1ULL << (r*8+f);
    for (int r=r0-1, f=f0-1; r>=1 && f>=1; r--, f--) mask |= 1ULL << (r*8+f);
    return mask;
}

// ---------------------------------------------------------------------------
// Occupancy enumeration
//
// Maps integer index i in [0, 2^popcount(mask)) to the i-th subset of the
// bits set in mask, using a carry-ripple technique.
// ---------------------------------------------------------------------------
static uint64_t indexToOccupancy(int index, uint64_t mask) {
    uint64_t occ = 0, m = mask;
    for (int i = 0; m; i++) {
        int bit = lsb(m); m &= m - 1;
        if (index & (1 << i)) occ |= 1ULL << bit;
    }
    return occ;
}

// ---------------------------------------------------------------------------
// Slow reference attack functions (used ONLY during table generation)
//
// propagateRook/Bishop with friendly=0, enemy=occ gives correct sliding
// attacks: rays extend until hitting a piece in occ (inclusive) or the edge.
// ---------------------------------------------------------------------------
static uint64_t slowRookAttacks  (int sq, uint64_t occ) { return Move::propagateRook  (1ULL << sq, 0, occ); }
static uint64_t slowBishopAttacks(int sq, uint64_t occ) { return Move::propagateBishop(1ULL << sq, 0, occ); }

// ---------------------------------------------------------------------------
// Magic number search
//
// Finds a 64-bit magic M for the given mask such that:
//     (occ * M) >> shift
// produces a unique index for every distinct subset of mask that results in
// a different attack bitboard.  "Constructive collisions" (two occupancies
// with the same attacks mapping to the same index) are fine.
//
// Sparse random candidates (AND of three randoms) are used because good magic
// numbers tend to have few set bits, which biases the multiplication toward
// spreading the high bits.
// ---------------------------------------------------------------------------
static uint64_t findMagic(int sq, uint64_t mask, bool rook) {
    int bits  = popcount(mask);
    int size  = 1 << bits;
    int shift = 64 - bits;

    // Materialise every occupancy subset and its corresponding attack bitboard.
    static uint64_t occs[4096], atks[4096];
    for (int i = 0; i < size; i++) {
        occs[i] = indexToOccupancy(i, mask);
        atks[i] = rook ? slowRookAttacks(sq, occs[i])
                       : slowBishopAttacks(sq, occs[i]);
    }

    // Sparse-random search with a per-square, per-piece-type seed for variety.
    static uint64_t used[4096];
    std::mt19937_64 rng(sq * 0xBF58476D1CE4E5B9ULL ^ (rook ? 0xDEADBEEFULL : 0xCAFEBABEULL));

    while (true) {
        uint64_t magic = rng() & rng() & rng();

        // Quick filter: the top byte of (mask * magic) should be dense.
        // Thin upper bytes correlate with poor index spreading.
        if (popcount((mask * magic) >> 56) < 6) continue;

        std::memset(used, 0xff, (size_t)size * sizeof(uint64_t)); // 0xff…f = empty sentinel
        bool fail = false;
        for (int i = 0; i < size && !fail; i++) {
            int idx = (int)((occs[i] * magic) >> shift);
            if (used[idx] == ~0ULL)
                used[idx] = atks[i];             // first occupancy to land here
            else if (used[idx] != atks[i])
                fail = true;                     // destructive collision → try next magic
            // constructive collision (same attack): silently accepted
        }
        if (!fail) return magic;
    }
}

// ---------------------------------------------------------------------------
// init() — call once at program start before any attack lookup
// ---------------------------------------------------------------------------
void init() {
    // --- Knight attacks ---
    for (int sq = 0; sq < 64; sq++)
        KNIGHT_ATTACKS[sq] = Move::propagateKnight(1ULL << sq, 0, 0);

    // --- King attacks ---
    for (int sq = 0; sq < 64; sq++)
        KING_ATTACKS[sq] = Move::propagateKing(1ULL << sq, 0, 0);

    // --- Pawn attacks (h1=bit0 geometry) ---
    // White moves toward higher ranks (+8 per rank).
    //   sq+9 = rank+1, file+1 (toward a-file)
    //   sq+7 = rank+1, file-1 (toward h-file)
    // Black is the mirror image with subtraction.
    for (int sq = 0; sq < 64; sq++) {
        uint64_t b    = 1ULL << sq;
        int      file = sq % 8;
        int      rank = sq / 8;

        uint64_t wAtk = 0;
        if (rank < 7) {                       // pawns never exist on rank 8
            if (file != 7) wAtk |= b << 9;   // not a-file edge
            if (file != 0) wAtk |= b << 7;   // not h-file edge
        }
        PAWN_ATTACKS[0][sq] = wAtk;

        uint64_t bAtk = 0;
        if (rank > 0) {                       // pawns never exist on rank 1
            if (file != 7) bAtk |= b >> 7;   // not a-file edge
            if (file != 0) bAtk |= b >> 9;   // not h-file edge
        }
        PAWN_ATTACKS[1][sq] = bAtk;
    }

    // --- Rook magic bitboards ---
    for (int sq = 0; sq < 64; sq++) {
        uint64_t mask  = rookMask(sq);
        int      bits  = popcount(mask);
        int      shift = 64 - bits;
        uint64_t magic = findMagic(sq, mask, true);

        ROOK_MAGICS[sq] = { mask, magic, shift, rookAttackStorage[sq] };

        int size = 1 << bits;
        for (int i = 0; i < size; i++) {
            uint64_t occ = indexToOccupancy(i, mask);
            int      idx = (int)((occ * magic) >> shift);
            rookAttackStorage[sq][idx] = slowRookAttacks(sq, occ);
        }
    }

    // --- Bishop magic bitboards ---
    for (int sq = 0; sq < 64; sq++) {
        uint64_t mask  = bishopMask(sq);
        int      bits  = popcount(mask);
        int      shift = 64 - bits;
        uint64_t magic = findMagic(sq, mask, false);

        BISHOP_MAGICS[sq] = { mask, magic, shift, bishopAttackStorage[sq] };

        int size = 1 << bits;
        for (int i = 0; i < size; i++) {
            uint64_t occ = indexToOccupancy(i, mask);
            int      idx = (int)((occ * magic) >> shift);
            bishopAttackStorage[sq][idx] = slowBishopAttacks(sq, occ);
        }
    }
}

} // namespace AttackTables
