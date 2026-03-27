#include "Board.h"
#include <iostream>

// ---------------------------------------------------------------------------
// Helper — extract commonly needed values from a Move
// ---------------------------------------------------------------------------
static std::tuple<uint64_t, uint64_t, uint64_t, int, int>
moveCharacteristics(const Move& move) {
    uint64_t from      = 1ULL << move.from;
    uint64_t to        = 1ULL << move.to;
    uint64_t fromTo    = from | to;
    int      colorIdx  = 7 * move.color;          // 0=white, 7=black
    int      pieceIdx  = colorIdx + move.piece + 1;
    return std::make_tuple(from, to, fromTo, colorIdx, pieceIdx);
}

static int countBits(uint64_t b) {
    int count = 0;
    while (b) { b &= b - 1; count++; }
    return count;
}

// ---------------------------------------------------------------------------
// Construction / initialisation
// ---------------------------------------------------------------------------
Board::Board() { init(); }

void Board::init() {
    whiteTurn        = true;
    halfMoveClock    = 0;
    enPassantSquare  = -1;
    fullMoveNumber   = 1;
    castleRights[0]  = castleRights[1] = castleRights[2] = castleRights[3] = true;
    moveHistory.clear();

    pieceBB[0]  = initWhite();
    pieceBB[1]  = initWhitePawns();
    pieceBB[2]  = initWhiteBishops();
    pieceBB[3]  = initWhiteKnights();
    pieceBB[4]  = initWhiteRooks();
    pieceBB[5]  = initWhiteQueens();
    pieceBB[6]  = initWhiteKing();
    pieceBB[7]  = initBlack();
    pieceBB[8]  = initBlackPawns();
    pieceBB[9]  = initBlackBishops();
    pieceBB[10] = initBlackKnights();
    pieceBB[11] = initBlackRooks();
    pieceBB[12] = initBlackQueens();
    pieceBB[13] = initBlackKing();

    occupiedBB = pieceBB[0] | pieceBB[7];
    emptyBB    = ~occupiedBB;
}

// ---------------------------------------------------------------------------
// Display helpers
// ---------------------------------------------------------------------------
std::string Board::displayBB(uint64_t bb) const {
    std::string s;
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            s += "|";
            s += ((1ULL << (63 - 8*i - j)) & bb) ? "X" : " ";
        }
        s += "|\n";
    }
    return s;
}

void Board::addPieces(uint64_t bb, std::string& boardString, char c) const {
    for (int i = 0; i < 64; i++) {
        if ((1ULL << (63 - i)) & bb) boardString[i] = c;
    }
}

std::string Board::constructBoardString() const {
    std::string s(64, ' ');
    addPieces(pieceBB[1],  s, 'p');
    addPieces(pieceBB[2],  s, 'b');
    addPieces(pieceBB[3],  s, 'n');
    addPieces(pieceBB[4],  s, 'r');
    addPieces(pieceBB[5],  s, 'q');
    addPieces(pieceBB[6],  s, 'k');
    addPieces(pieceBB[8],  s, 'P');
    addPieces(pieceBB[9],  s, 'B');
    addPieces(pieceBB[10], s, 'N');
    addPieces(pieceBB[11], s, 'R');
    addPieces(pieceBB[12], s, 'Q');
    addPieces(pieceBB[13], s, 'K');
    return s;
}

std::string Board::displayBoard() const {
    std::string out;
    std::string bs = constructBoardString();
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) { out += "|"; out += bs[8*i+j]; }
        out += "|\n";
    }
    return out;
}

std::string Board::displayBoard(uint64_t bb) const {
    std::string bs = constructBoardString();
    addPieces(bb, bs, 'X');
    std::string out;
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) { out += "|"; out += bs[8*i+j]; }
        out += "|\n";
    }
    return out;
}

// ---------------------------------------------------------------------------
// Attack board
// ---------------------------------------------------------------------------
uint64_t Board::attackBoard(bool white) const {
    uint64_t friendly, enemy, pawns, bishops, knights, rooks, queens, king;

    uint64_t centerPawns = 0x7e7e7e7e7e7e00ULL;
    uint64_t leftPawns   = 0x80808080808000ULL;
    uint64_t rightPawns  = 0x01010101010100ULL;

    if (white) {
        friendly = pieceBB[0];  enemy   = pieceBB[7];
        pawns    = pieceBB[1];  bishops = pieceBB[2];
        knights  = pieceBB[3];  rooks   = pieceBB[4];
        queens   = pieceBB[5];  king    = pieceBB[6];
        centerPawns &= pawns; leftPawns &= pawns; rightPawns &= pawns;
        uint64_t pawnAtk = (centerPawns << 9) | (centerPawns << 7) |
                           (leftPawns   << 7) | (rightPawns  << 9);
        pawnAtk &= ~friendly;
        return pawnAtk
             | Move::propagateBishop(bishops, friendly, enemy)
             | Move::propagateKnight(knights, friendly, enemy)
             | Move::propagateRook  (rooks,   friendly, enemy)
             | Move::propagateQueen (queens,  friendly, enemy)
             | Move::propagateKing  (king,    friendly, enemy);
    } else {
        friendly = pieceBB[7];  enemy   = pieceBB[0];
        pawns    = pieceBB[8];  bishops = pieceBB[9];
        knights  = pieceBB[10]; rooks   = pieceBB[11];
        queens   = pieceBB[12]; king    = pieceBB[13];
        centerPawns &= pawns; leftPawns &= pawns; rightPawns &= pawns;
        uint64_t pawnAtk = (centerPawns >> 9) | (centerPawns >> 7) |
                           (leftPawns   >> 9) | (rightPawns  >> 7);
        return pawnAtk
             | Move::propagateBishop(bishops, friendly, enemy)
             | Move::propagateKnight(knights, friendly, enemy)
             | Move::propagateRook  (rooks,   friendly, enemy)
             | Move::propagateQueen (queens,  friendly, enemy)
             | Move::propagateKing  (king,    friendly, enemy);
    }
}

// ---------------------------------------------------------------------------
// Move notation
// ---------------------------------------------------------------------------
std::string Board::moveNotation(const Move& move) {
    std::ostringstream sb;
    if (move.moveType == Move::Castle) {
        sb << (move.rookSide == Move::Kingside ? "O-O" : "O-O-O");
        return sb.str();
    }
    sb << Move::pieceNotiation(move.piece);
    int index = whiteTurn ? 0 : 7;
    switch (move.piece) {
        case Move::Bishop: { int cnt = countBits(pieceBB[index+2]); if (cnt >= 2) { /* TODO disambiguate */ } break; }
        case Move::Knight: { int cnt = countBits(pieceBB[index+3]); if (cnt >= 2) { /* TODO disambiguate */ } break; }
        case Move::Rook:   { int cnt = countBits(pieceBB[index+4]); if (cnt >= 2) { /* TODO disambiguate */ } break; }
        case Move::Queen:  { int cnt = countBits(pieceBB[index+5]); if (cnt >= 2) { /* TODO disambiguate */ } break; }
        default: break;
    }
    if (move.moveType == Move::Capture || move.moveType == Move::PromoteCapture) sb << "x";
    if (move.moveType == Move::Promote) {
        sb << "=" << Move::pieceNotiation(move.newPiece);
    }
    sb << Move::bitboardPositionToNotation(move.to);
    if (move.moveType == Move::PromoteCapture) {
        sb << "=" << Move::pieceNotiation(move.newPiece);
    }
    if (check()) {
        sb << (checkMate() ? "#" : "+");
    }
    return sb.str();
}

// ---------------------------------------------------------------------------
// Check / checkmate / stalemate
// ---------------------------------------------------------------------------
bool Board::isKingInCheck(bool white) const {
    uint64_t opponentAttacks = attackBoard(!white);
    uint64_t king            = white ? pieceBB[6] : pieceBB[13];
    return (opponentAttacks & king) != 0;
}

bool Board::check() const {
    // Is the side-to-move's king under attack?
    return isKingInCheck(whiteTurn);
}

bool Board::checkMate() {
    if (!check()) return false;
    // TODO: requires generateMoves (Phase 4)
    return false;
}

bool Board::staleMate() {
    // TODO: requires generateMoves (Phase 4)
    return false;
}

// ---------------------------------------------------------------------------
// Move application — specialisations
// ---------------------------------------------------------------------------

// --- Quiet move ---
template<>
void Board::updateByMove<Move::Quiet>(Move move) {
    auto [from, to, fromTo, colorIdx, pieceIdx] = moveCharacteristics(move);

    pieceBB[pieceIdx] ^= fromTo;
    pieceBB[colorIdx] ^= fromTo;
    occupiedBB        ^= fromTo;
    emptyBB           ^= fromTo;

    if (move.piece != Move::Pawn) halfMoveClock++;
    else                          halfMoveClock = 0;

    enPassantSquare = -1;
    // Track double pawn push for en passant
    if (move.piece == Move::Pawn && (move.to - move.from == 16 || move.from - move.to == 16)) {
        enPassantSquare = (move.from + move.to) / 2;
    }

    if (move.piece == Move::King) {
        castleRights[2 * move.color]     = false;
        castleRights[2 * move.color + 1] = false;
    } else if (move.piece == Move::Rook) {
        // h-file rooks (short/kingside): bit 0 (h1) and bit 56 (h8)
        uint64_t shortRookSquares = (1ULL << 56) | 1ULL;
        int shortCastle = (int)((shortRookSquares & from) != 0);
        castleRights[2 * move.color + shortCastle] = false;
    }

    if (move.color == Move::Black) fullMoveNumber++;
    whiteTurn = !whiteTurn;
    moveHistory.push_back(move);
}

// --- Capture ---
template<>
void Board::updateByMove<Move::Capture>(Move move) {
    auto [from, to, fromTo, colorIdx, pieceIdx] = moveCharacteristics(move);
    int enemyColorIdx   = 7 - colorIdx;  // 7 for white attacker, 0 for black attacker
    int capturedPieceIdx = enemyColorIdx + move.cPiece + 1;

    pieceBB[pieceIdx]        ^= fromTo; // move attacker
    pieceBB[colorIdx]        ^= fromTo;
    pieceBB[capturedPieceIdx] ^= to;    // remove captured piece
    pieceBB[enemyColorIdx]    ^= to;

    occupiedBB = pieceBB[0] | pieceBB[7];
    emptyBB    = ~occupiedBB;

    halfMoveClock   = 0;
    enPassantSquare = -1;

    // Strip castle rights if a rook was captured on its starting square
    if (move.cPiece == Move::Rook) {
        uint64_t shortRookSquares = (1ULL << 56) | 1ULL;
        int shortCastle = (int)((shortRookSquares & to) != 0);
        castleRights[2 * move.cColor + shortCastle] = false;
    }

    if (move.piece == Move::King) {
        castleRights[2 * move.color]     = false;
        castleRights[2 * move.color + 1] = false;
    } else if (move.piece == Move::Rook) {
        uint64_t shortRookSquares = (1ULL << 56) | 1ULL;
        int shortCastle = (int)((shortRookSquares & from) != 0);
        castleRights[2 * move.color + shortCastle] = false;
    }

    if (move.color == Move::Black) fullMoveNumber++;
    whiteTurn = !whiteTurn;
    moveHistory.push_back(move);
}

// --- Promotion ---
template<>
void Board::updateByMove<Move::Promote>(Move move) {
    auto [from, to, fromTo, colorIdx, pieceIdx] = moveCharacteristics(move);
    int newPieceIdx = colorIdx + move.newPiece + 1;

    // Remove pawn at 'from', add promoted piece at 'to'
    pieceBB[pieceIdx]  &= ~from;
    pieceBB[newPieceIdx] |= to;
    pieceBB[colorIdx]  ^= fromTo;

    occupiedBB = pieceBB[0] | pieceBB[7];
    emptyBB    = ~occupiedBB;

    halfMoveClock   = 0;
    enPassantSquare = -1;

    if (move.color == Move::Black) fullMoveNumber++;
    whiteTurn = !whiteTurn;
    moveHistory.push_back(move);
}

// --- Promote-capture ---
template<>
void Board::updateByMove<Move::PromoteCapture>(Move move) {
    auto [from, to, fromTo, colorIdx, pieceIdx] = moveCharacteristics(move);
    int newPieceIdx      = colorIdx + move.newPiece + 1;
    int enemyColorIdx    = 7 - colorIdx;
    int capturedPieceIdx = enemyColorIdx + move.cPiece + 1;

    // Remove captured piece first, then promote
    pieceBB[capturedPieceIdx] ^= to;
    pieceBB[enemyColorIdx]    ^= to;
    pieceBB[pieceIdx]         &= ~from;
    pieceBB[newPieceIdx]      |= to;
    pieceBB[colorIdx]         ^= fromTo;

    occupiedBB = pieceBB[0] | pieceBB[7];
    emptyBB    = ~occupiedBB;

    halfMoveClock   = 0;
    enPassantSquare = -1;

    if (move.color == Move::Black) fullMoveNumber++;
    whiteTurn = !whiteTurn;
    moveHistory.push_back(move);
}

// --- Castle ---
// Square indices for h1=bit0 layout:
//   White O-O:   king 3→1, rook 0→2
//   White O-O-O: king 3→5, rook 7→4
//   Black O-O:   king 59→57, rook 56→58
//   Black O-O-O: king 59→61, rook 63→60
template<>
void Board::updateByMove<Move::Castle>(Move move) {
    int colorIdx = 7 * move.color;
    int kingIdx  = colorIdx + Move::King + 1;  // pieceBB index for king
    int rookIdx  = colorIdx + Move::Rook + 1;  // pieceBB index for rooks

    int kingFrom, kingTo, rookFrom, rookTo;
    if (move.color == Move::White) {
        kingFrom = 3;
        if (move.rookSide == Move::Kingside)  { kingTo = 1; rookFrom = 0; rookTo = 2; }
        else                                  { kingTo = 5; rookFrom = 7; rookTo = 4; }
    } else {
        kingFrom = 59;
        if (move.rookSide == Move::Kingside)  { kingTo = 57; rookFrom = 56; rookTo = 58; }
        else                                  { kingTo = 61; rookFrom = 63; rookTo = 60; }
    }

    uint64_t kingFromTo = (1ULL << kingFrom) | (1ULL << kingTo);
    uint64_t rookFromTo = (1ULL << rookFrom) | (1ULL << rookTo);
    uint64_t allFromTo  = kingFromTo | rookFromTo;

    pieceBB[kingIdx]  ^= kingFromTo;
    pieceBB[rookIdx]  ^= rookFromTo;
    pieceBB[colorIdx] ^= allFromTo;
    occupiedBB        ^= allFromTo;
    emptyBB           ^= allFromTo;

    castleRights[2 * move.color]     = false;
    castleRights[2 * move.color + 1] = false;
    halfMoveClock++;
    enPassantSquare = -1;

    if (move.color == Move::Black) fullMoveNumber++;
    whiteTurn = !whiteTurn;
    moveHistory.push_back(move);
}
