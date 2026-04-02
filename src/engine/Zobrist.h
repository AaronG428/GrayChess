#pragma once
#include <cstdint>

class Board; // forward declaration at global scope so Zobrist functions use ::Board

// Zobrist hashing — header-only.
// All keys are populated once by Zobrist::init() at program startup.
// Board::hash is maintained incrementally by XOR-ing keys in/out
// inside every updateByMove specialization and unmakeMove.
namespace Zobrist {

    // One random key per (pieceBB index 0-13, square 0-63)
    // Leave PIECE_KEYS[0] and [7] empty, not needed for hashing but useful to keep for indexing
    extern uint64_t PIECE_KEYS[14][64];

    // XOR'd into the hash whenever it is white's turn to move
    extern uint64_t SIDE_KEY;

    // Indexed by the 4-bit castle-rights word  (bit0=wLong, bit1=wShort,
    // bit2=bLong, bit3=bShort)
    extern uint64_t CASTLE_KEYS[16];

    // Indexed by en-passant file (0=h … 7=a); use 0 when there is no ep square
    extern uint64_t EP_KEYS[8];

    // Seed all key arrays with pseudo-random values.
    // Must be called once before any Board is constructed.
    void init();

    // Return the hash for a board built from scratch (used by loadFEN / init).

    uint64_t compute(const Board& board);

    // Return the XOR delta for moving a piece (quiet move).
    inline uint64_t movePiece(int pieceIdx, int from, int to) {
        return PIECE_KEYS[pieceIdx][from] ^ PIECE_KEYS[pieceIdx][to];
    }

    // Return the XOR delta for removing a piece from a square.
    inline uint64_t removePiece(int pieceIdx, int sq) {
        return PIECE_KEYS[pieceIdx][sq];
    }

    // Return the XOR delta for the castle-rights change old→new.
    inline uint64_t castleRightsDelta(int oldRights, int newRights) {
        return CASTLE_KEYS[oldRights] ^ CASTLE_KEYS[newRights]; 
    }

    // Return the XOR delta for an en-passant file change old→new.
    // Pass -1 for "no ep square".
    inline uint64_t epFileDelta(int oldEpSq, int newEpSq) {
        uint64_t delta = 0;
        if (oldEpSq != -1) delta ^= EP_KEYS[oldEpSq%8];
        if (newEpSq != -1) delta ^= EP_KEYS[newEpSq%8];
        return delta;
    }

    inline uint64_t switchSide() {
        return SIDE_KEY;
    }

} // namespace Zobrist
