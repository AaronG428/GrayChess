#pragma once
#include <cstdint>
#include <string>
#include <sstream>

class Move {
public:
    enum PieceEnum { Pawn, Bishop, Knight, Rook, Queen, King };
    enum ColorEnum  { White, Black };
    enum MoveEnum   { Quiet, Castle, Capture, Promote, PromoteCapture };
    enum RookSide   { Kingside, Queenside };

    MoveEnum  moveType;
    int       from;
    int       to;
    PieceEnum piece;
    ColorEnum color;
    PieceEnum cPiece;  // captured piece type
    ColorEnum cColor;  // captured piece color
    RookSide  rookSide;
    PieceEnum newPiece; // for promotion

    static std::string toString(const Move& move);
    static std::string bitboardPositionToNotation(int position);
    static std::string pieceNotiation(PieceEnum piece);

    static uint64_t propagateDirection(uint64_t b, uint64_t friendly = 0, uint64_t enemy = 0,
                                       uint64_t direction = 0, bool limit = false);
    static uint64_t propagateBishop(uint64_t b, uint64_t friendly = 0, uint64_t enemy = 0,
                                    bool limit = false);
    static uint64_t propagateRook(uint64_t b, uint64_t friendly = 0, uint64_t enemy = 0,
                                  bool limit = false);
    static uint64_t propagateQueen(uint64_t b, uint64_t friendly = 0, uint64_t enemy = 0,
                                   bool limit = false);
    static uint64_t propagateKing(uint64_t b, uint64_t friendly = 0, uint64_t enemy = 0);
    static uint64_t propagateKnight(uint64_t b, uint64_t friendly = 0, uint64_t enemy = 0);
};
