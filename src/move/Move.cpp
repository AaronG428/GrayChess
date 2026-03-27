#include "Move.h"

std::string Move::toString(const Move& move) {
    std::ostringstream sb;
    sb << "Move: ";
    sb << "from " << move.from << " to " << move.to << ", ";
    sb << "piece " << move.piece << ", ";
    sb << "color " << move.color << ", ";
    sb << "captured piece " << move.cPiece << ", ";
    sb << "captured color " << move.cColor << ", ";
    sb << "rook side " << move.rookSide << ", ";
    sb << "new piece " << move.newPiece;
    return sb.str();
}

// Board orientation: h1=bit0, a1=bit7, h8=bit56, a8=bit63
// position % 8 == 0 → h-file, position % 8 == 7 → a-file
std::string Move::bitboardPositionToNotation(int position) {
    char file = 'a' + (7 - (position % 8));
    char rank = '1' + (position / 8);
    return std::string(1, file) + std::string(1, rank);
}

std::string Move::pieceNotiation(PieceEnum piece) {
    switch (piece) {
        case Bishop: return "B";
        case Knight: return "N";
        case Rook:   return "R";
        case Queen:  return "Q";
        case King:   return "K";
        default:     return "";
    }
}

// Direction guide:
// |7|6|5|
// |4|X|3|
// |2|1|0|
uint64_t Move::propagateDirection(uint64_t b, uint64_t friendly, uint64_t enemy,
                                   uint64_t direction, bool limit) {
    if (b == 0) return 0;

    uint64_t dwnCap   = 0xff;
    uint64_t upCap    = dwnCap << 56;
    uint64_t rightCap = 0x0101010101010101ULL;
    uint64_t leftCap  = rightCap << 7;

    uint64_t DOWN_BITS  = 0b00000111; // dirs {0,1,2}
    uint64_t UP_BITS    = 0b11100000; // dirs {5,6,7}
    uint64_t RIGHT_BITS = 0b00101001; // dirs {0,3,5}
    uint64_t LEFT_BITS  = 0b10010100; // dirs {2,4,7}

    int d = direction & 7;
    uint64_t cap  = ((DOWN_BITS  >> d) & 1) * dwnCap;
    cap |= ((UP_BITS    >> d) & 1) * upCap;
    cap |= ((RIGHT_BITS >> d) & 1) * rightCap;
    cap |= ((LEFT_BITS  >> d) & 1) * leftCap;

    uint64_t safe_b = b & ~(cap | enemy);

    bool shiftUp    = direction > 3;
    bool levelShift = (direction != 3) & (direction != 4);
    int  shiftAmt   = 1;
    if (levelShift) { shiftAmt = shiftUp ? direction + 2 : 9 - direction; }

    uint64_t next = shiftUp ? safe_b << shiftAmt : safe_b >> shiftAmt;
    next &= ~friendly;

    if (limit) return next;
    return next | propagateDirection(next, friendly, enemy, direction);
}

uint64_t Move::propagateBishop(uint64_t b, uint64_t friendly, uint64_t enemy, bool limit) {
    return propagateDirection(b, friendly, enemy, 7, limit) |
           propagateDirection(b, friendly, enemy, 5, limit) |
           propagateDirection(b, friendly, enemy, 2, limit) |
           propagateDirection(b, friendly, enemy, 0, limit);
}

uint64_t Move::propagateRook(uint64_t b, uint64_t friendly, uint64_t enemy, bool limit) {
    return propagateDirection(b, friendly, enemy, 6, limit) |
           propagateDirection(b, friendly, enemy, 4, limit) |
           propagateDirection(b, friendly, enemy, 3, limit) |
           propagateDirection(b, friendly, enemy, 1, limit);
}

uint64_t Move::propagateQueen(uint64_t b, uint64_t friendly, uint64_t enemy, bool limit) {
    return propagateRook(b, friendly, enemy, limit) |
           propagateBishop(b, friendly, enemy, limit);
}

uint64_t Move::propagateKing(uint64_t b, uint64_t friendly, uint64_t enemy) {
    return propagateQueen(b, friendly, enemy, true);
}

uint64_t Move::propagateKnight(uint64_t b, uint64_t friendly, uint64_t enemy) {
    // Knight movement (see Move.h for diagram)
    uint64_t dwnCap      = 0xffULL;
    uint64_t upCap       = dwnCap << 56;
    uint64_t rightCap    = 0x0101010101010101ULL;
    uint64_t leftCap     = rightCap << 7;
    uint64_t twoDwnCap   = dwnCap << 8;
    uint64_t twoUpCap    = upCap >> 8;
    uint64_t twoRightCap = rightCap << 1;
    uint64_t twoLeftCap  = leftCap >> 1;

    uint64_t DOWN_BITS      = 0b00001111;
    uint64_t UP_BITS        = 0b11110000;
    uint64_t RIGHT_BITS     = 0b01010101;
    uint64_t LEFT_BITS      = 0b10101010;
    uint64_t TWO_DOWN_BITS  = 0b00000011;
    uint64_t TWO_UP_BITS    = 0b11000000;
    uint64_t TWO_RIGHT_BITS = 0b00010100;
    uint64_t TWO_LEFT_BITS  = 0b00101000;

    int shiftAmts[8] = {-17, -15, -10, -6, 6, 10, 15, 17};
    uint64_t next = 0;

    for (int d = 0; d < 8; d++) {
        uint64_t cap  = ((DOWN_BITS      >> d) & 1) * dwnCap;
        cap |= ((UP_BITS        >> d) & 1) * upCap;
        cap |= ((RIGHT_BITS     >> d) & 1) * rightCap;
        cap |= ((LEFT_BITS      >> d) & 1) * leftCap;
        cap |= ((TWO_DOWN_BITS  >> d) & 1) * twoDwnCap;
        cap |= ((TWO_UP_BITS    >> d) & 1) * twoUpCap;
        cap |= ((TWO_RIGHT_BITS >> d) & 1) * twoRightCap;
        cap |= ((TWO_LEFT_BITS  >> d) & 1) * twoLeftCap;
        uint64_t safe_b = b & ~cap;
        int s = shiftAmts[d];
        next |= s < 0 ? safe_b >> -s : safe_b << s;
    }
    next &= ~friendly;
    return next;
}
