#pragma once
#include <cstdint>
#include <string>
#include <sstream>
#include <tuple>
#include <vector>
#include "../move/Move.h"

// Board orientation:
//   h1 = bit 0 (LSB), a1 = bit 7, h8 = bit 56, a8 = bit 63
//
// pieceBB layout:
//   [0]  = white (all)
//   [1]  = white pawns
//   [2]  = white bishops
//   [3]  = white knights
//   [4]  = white rooks
//   [5]  = white queens
//   [6]  = white king
//   [7]  = black (all)
//   [8]  = black pawns
//   [9]  = black bishops
//   [10] = black knights
//   [11] = black rooks
//   [12] = black queens
//   [13] = black king
//
// colorIndex = 7 * color  (0 for white, 7 for black)
// pieceIndex = colorIndex + piece + 1

class Board {
public:
    bool     whiteTurn;
    int      halfMoveClock;
    bool     castleRights[4]; // [wLong, wShort, bLong, bShort]
    int      enPassantSquare; // -1 if none, else the ep-target square index
    int      fullMoveNumber;

    uint64_t hash = 0;   // Zobrist hash — maintained incrementally (Phase 8)

    uint64_t pieceBB[14];
    uint64_t emptyBB;
    uint64_t occupiedBB;

    std::vector<Move> moveHistory;

    Board();
    void init();
    void loadFEN(const std::string& fen);

    // Display
    std::string displayBB(uint64_t bb) const;
    void        addPieces(uint64_t bb, std::string& boardString, char c) const;
    std::string constructBoardString() const;
    std::string displayBoard() const;
    std::string displayBoard(uint64_t bb) const;

    // Game logic
    uint64_t    attackBoard(bool white) const;
    std::string moveNotation(const Move& move);
    bool        check() const;
    bool     oppCheck() const;
    bool checkMate();
    bool        staleMate();

    // Move application — declare only; all specialisations are defined in Board.cpp
    template<Move::MoveEnum moveType>
    void updateByMove(Move move);

    // Explicit specialisation declarations (required so callers don't use the
    // uninstantiable primary template)
    template<> void updateByMove<Move::Quiet>(Move move);
    template<> void updateByMove<Move::Castle>(Move move);
    template<> void updateByMove<Move::Capture>(Move move);
    template<> void updateByMove<Move::Promote>(Move move);
    template<> void updateByMove<Move::PromoteCapture>(Move move);
    template<> void updateByMove<Move::EnPassant>(Move move);

    void unmakeMove();

    // Runtime dispatch — applies any move type without a compile-time template argument.
    // Used by legality filtering and the search loop.
    void applyMove(const Move& m);

private:
    uint64_t initWhite()        const { return 0xffffULL; }
    uint64_t initWhitePawns()   const { return 0xff00ULL; }
    uint64_t initWhiteBishops() const { return 0x24ULL; }
    uint64_t initWhiteKnights() const { return 0x42ULL; }
    uint64_t initWhiteRooks()   const { return 0x81ULL; }
    uint64_t initWhiteQueens()  const { return 0x10ULL; }
    uint64_t initWhiteKing()    const { return 0x08ULL; }
    uint64_t initBlack()        const { return 0xffffULL << 48; }
    uint64_t initBlackPawns()   const { return 0x00ffULL << 48; }
    uint64_t initBlackBishops() const { return 0x2400ULL << 48; }
    uint64_t initBlackKnights() const { return 0x4200ULL << 48; }
    uint64_t initBlackRooks()   const { return 0x8100ULL << 48; }
    uint64_t initBlackQueens()  const { return 0x1000ULL << 48; }
    uint64_t initBlackKing()    const { return 0x0800ULL << 48; }
};
