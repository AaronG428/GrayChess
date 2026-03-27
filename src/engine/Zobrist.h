#pragma once
#include <cstdint>

class Board; // forward declaration at global scope so Zobrist functions use ::Board

// Zobrist hashing — header-only.
// All keys are populated once by Zobrist::init() at program startup.
// Board::hash is maintained incrementally by XOR-ing keys in/out
// inside every updateByMove specialization and unmakeMove.
namespace Zobrist {

    // One random key per (pieceBB index 0-13, square 0-63)
    extern uint64_t PIECE_KEYS[14][64];

    // XOR'd into the hash whenever it is black's turn to move
    extern uint64_t SIDE_KEY;

    // Indexed by the 4-bit castle-rights word  (bit0=wLong, bit1=wShort,
    // bit2=bLong, bit3=bShort)
    extern uint64_t CASTLE_KEYS[16];

    // Indexed by en-passant file (0=a … 7=h); use 0 when there is no ep square
    extern uint64_t EP_KEYS[8];

    // Seed all key arrays with pseudo-random values.
    // Must be called once before any Board is constructed.
    void init();

    // Return the hash for a board built from scratch (used by loadFEN / init).
    // TODO Phase 8: XOR all occupied squares + side + castle + ep
    uint64_t compute(const Board& board);

    // Return the XOR delta for moving a piece (quiet move).
    // TODO Phase 8: PIECE_KEYS[pieceIdx][from] ^ PIECE_KEYS[pieceIdx][to]
    inline uint64_t movePiece(int pieceIdx, int from, int to) {
        (void)pieceIdx; (void)from; (void)to;
        return 0; // TODO
    }

    // Return the XOR delta for removing a piece from a square.
    // TODO Phase 8: PIECE_KEYS[pieceIdx][sq]
    inline uint64_t removePiece(int pieceIdx, int sq) {
        (void)pieceIdx; (void)sq;
        return 0; // TODO
    }

    // Return the XOR delta for the castle-rights change old→new.
    // TODO Phase 8: CASTLE_KEYS[oldRights] ^ CASTLE_KEYS[newRights]
    inline uint64_t castleRightsDelta(int oldRights, int newRights) {
        (void)oldRights; (void)newRights;
        return 0; // TODO
    }

    // Return the XOR delta for an en-passant file change old→new.
    // Pass -1 for "no ep square".
    // TODO Phase 8: EP_KEYS[oldFile] ^ EP_KEYS[newFile]  (skip if -1)
    inline uint64_t epFileDelta(int oldEpSq, int newEpSq) {
        (void)oldEpSq; (void)newEpSq;
        return 0; // TODO
    }

} // namespace Zobrist
