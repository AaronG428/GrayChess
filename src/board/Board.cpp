#include "Board.h"
#include "../move/AttackTables.h"
#include "../move/MoveGenerator.h"
#include "../utils/Bits.h"
#include "../engine/Zobrist.h"
#include <iostream>

using namespace std;

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------
static std::tuple<uint64_t, uint64_t, uint64_t, int, int>
moveCharacteristics(const Move& move) {
    uint64_t from     = 1ULL << move.from;
    uint64_t to       = 1ULL << move.to;
    uint64_t fromTo   = from | to;
    int colorIdx      = 7 * move.color;          // 0=white, 7=black
    int pieceIdx      = colorIdx + move.piece + 1;
    return std::make_tuple(from, to, fromTo, colorIdx, pieceIdx);
}

static int countBits(uint64_t b) {
    int c = 0; while (b) { b &= b - 1; c++; } return c;
}

// Save board metadata onto a move before mutating — used by unmakeMove()
static void savePrevState(Move& move, const Board& b) {
    move.prevEnPassantSquare = b.enPassantSquare;
    move.prevHalfMoveClock   = b.halfMoveClock;
    move.prevCastleRights[0] = b.castleRights[0];
    move.prevCastleRights[1] = b.castleRights[1];
    move.prevCastleRights[2] = b.castleRights[2];
    move.prevCastleRights[3] = b.castleRights[3];
}

// ---------------------------------------------------------------------------
// Construction / initialisation
// ---------------------------------------------------------------------------
Board::Board() { init(); }

void Board::loadFEN(const std::string& fen) {
    // Clear all piece bitboards and metadata
    for (int i = 0; i < 14; i++) pieceBB[i] = 0;
    castleRights[0] = castleRights[1] = castleRights[2] = castleRights[3] = false;
    enPassantSquare = -1;
    halfMoveClock   = 0;
    fullMoveNumber  = 1;
    moveHistory.clear();

    std::istringstream ss(fen);
    std::string placement, activeColor, castling, epStr;
    ss >> placement >> activeColor >> castling >> epStr;

    // --- Piece placement ---
    // FEN describes ranks 8 down to 1, files a to h.
    // Engine square: sq = (rank-1)*8 + (7-file)  where file: a=0, h=7.
    int rank = 8, file = 0;
    for (char c : placement) {
        if (c == '/') {
            rank--;
            file = 0;
        } else if (c >= '1' && c <= '8') {
            file += c - '0';
        } else {
            int sq = (rank - 1) * 8 + (7 - file);
            int colorIdx, pieceEnum;
            switch (c) {
                case 'P': colorIdx=0; pieceEnum=Move::Pawn;   break;
                case 'B': colorIdx=0; pieceEnum=Move::Bishop; break;
                case 'N': colorIdx=0; pieceEnum=Move::Knight; break;
                case 'R': colorIdx=0; pieceEnum=Move::Rook;   break;
                case 'Q': colorIdx=0; pieceEnum=Move::Queen;  break;
                case 'K': colorIdx=0; pieceEnum=Move::King;   break;
                case 'p': colorIdx=7; pieceEnum=Move::Pawn;   break;
                case 'b': colorIdx=7; pieceEnum=Move::Bishop; break;
                case 'n': colorIdx=7; pieceEnum=Move::Knight; break;
                case 'r': colorIdx=7; pieceEnum=Move::Rook;   break;
                case 'q': colorIdx=7; pieceEnum=Move::Queen;  break;
                case 'k': colorIdx=7; pieceEnum=Move::King;   break;
                default:  colorIdx=0; pieceEnum=0;            break;
            }
            pieceBB[colorIdx + pieceEnum + 1] |= (1ULL << sq);
            file++;
        }
    }

    // Rebuild color aggregates
    pieceBB[0] = 0; for (int i = 1; i <= 6; i++) pieceBB[0] |= pieceBB[i];
    pieceBB[7] = 0; for (int i = 8; i <= 13; i++) pieceBB[7] |= pieceBB[i];
    occupiedBB = pieceBB[0] | pieceBB[7];
    emptyBB    = ~occupiedBB;

    // --- Active color ---
    whiteTurn = (activeColor == "w");

    // --- Castling rights ---
    for (char c : castling) {
        switch (c) {
            case 'K': castleRights[1] = true; break; // white short
            case 'Q': castleRights[0] = true; break; // white long
            case 'k': castleRights[3] = true; break; // black short
            case 'q': castleRights[2] = true; break; // black long
        }
    }

    // --- En passant ---
    if (epStr.size() >= 2 && epStr[0] != '-') {
        int epFile = epStr[0] - 'a';       // a=0 … h=7
        int epRank = epStr[1] - '0';       // '1'=1 … '8'=8
        enPassantSquare = (epRank - 1) * 8 + (7 - epFile);
    }

    // --- Halfmove clock and fullmove number (optional in FEN) ---
    int hm = 0, fm = 1;
    ss >> hm >> fm;
    halfMoveClock  = hm;
    fullMoveNumber = fm;

    // --- Zobrist hash ---
    hash = Zobrist::compute(*this);
}

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

    hash = Zobrist::compute(*this);
}

// ---------------------------------------------------------------------------
// Display
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
    for (int i = 0; i < 64; i++)
        if ((1ULL << (63 - i)) & bb) boardString[i] = c;
}

std::string Board::constructBoardString() const {
    std::string s(64, ' ');
    addPieces(pieceBB[1],  s, 'p');  // white pieces — lowercase
    addPieces(pieceBB[2],  s, 'b');
    addPieces(pieceBB[3],  s, 'n');
    addPieces(pieceBB[4],  s, 'r');
    addPieces(pieceBB[5],  s, 'q');
    addPieces(pieceBB[6],  s, 'k');
    addPieces(pieceBB[8],  s, 'P');  // black pieces — uppercase
    addPieces(pieceBB[9],  s, 'B');
    addPieces(pieceBB[10], s, 'N');
    addPieces(pieceBB[11], s, 'R');
    addPieces(pieceBB[12], s, 'Q');
    addPieces(pieceBB[13], s, 'K');
    return s;
}

std::string Board::displayBoard() const {
    std::string out, bs = constructBoardString();
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
    int colorIdx = white ? 0 : 7;
    uint64_t pawns   = pieceBB[colorIdx + 1];
    uint64_t bishops = pieceBB[colorIdx + 2];
    uint64_t knights = pieceBB[colorIdx + 3];
    uint64_t rooks   = pieceBB[colorIdx + 4];
    uint64_t queens  = pieceBB[colorIdx + 5];
    uint64_t king    = pieceBB[colorIdx + 6];

    uint64_t attacks = 0;
    int colorEnum = white ? 0 : 1;

    // Pawns — use pre-computed per-square pawn attack tables
    {
        uint64_t bb = pawns;
        while (bb) { int sq = lsb(bb); bb &= bb - 1; attacks |= AttackTables::PAWN_ATTACKS[colorEnum][sq]; }
    }
    // Knights
    {
        uint64_t bb = knights;
        while (bb) { int sq = lsb(bb); bb &= bb - 1; attacks |= AttackTables::KNIGHT_ATTACKS[sq]; }
    }
    // Bishops
    {
        uint64_t bb = bishops;
        while (bb) { int sq = lsb(bb); bb &= bb - 1; attacks |= AttackTables::bishopAttacks(sq, occupiedBB); }
    }
    // Rooks
    {
        uint64_t bb = rooks;
        while (bb) { int sq = lsb(bb); bb &= bb - 1; attacks |= AttackTables::rookAttacks(sq, occupiedBB); }
    }
    // Queens
    {
        uint64_t bb = queens;
        while (bb) { int sq = lsb(bb); bb &= bb - 1; attacks |= AttackTables::queenAttacks(sq, occupiedBB); }
    }
    // King
    {
        uint64_t bb = king;
        while (bb) { int sq = lsb(bb); bb &= bb - 1; attacks |= AttackTables::KING_ATTACKS[sq]; }
    }

    return attacks;
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
    // Use move.color to look up the right pieces (whiteTurn has already toggled)
    int index = (move.color == Move::White) ? 0 : 7;
    switch (move.piece) {
        case Move::Bishop: { if (countBits(pieceBB[index+2]) >= 2) { /* TODO disambiguate */ } break; }
        case Move::Knight: { if (countBits(pieceBB[index+3]) >= 2) { /* TODO disambiguate */ } break; }
        case Move::Rook:   { if (countBits(pieceBB[index+4]) >= 2) { /* TODO disambiguate */ } break; }
        case Move::Queen:  { if (countBits(pieceBB[index+5]) >= 2) { /* TODO disambiguate */ } break; }
        default: break;
    }
    if (move.moveType == Move::Capture     ||
        move.moveType == Move::PromoteCapture ||
        move.moveType == Move::EnPassant)   sb << "x";
    if (move.moveType == Move::Promote || move.moveType == Move::PromoteCapture)
        sb << "=" << Move::pieceNotiation(move.newPiece);
    sb << Move::bitboardPositionToNotation(move.to);
    if (move.moveType == Move::EnPassant) sb << " e.p.";
    if (check()) sb << (checkMate() ? "#" : "+");
    return sb.str();
}

// ---------------------------------------------------------------------------
// Check / checkmate / stalemate
// ---------------------------------------------------------------------------
bool Board::isKingInCheck(bool white) const {
    uint64_t opponentAtks = attackBoard(!white);
    uint64_t king         = white ? pieceBB[6] : pieceBB[13];
    return (opponentAtks & king) != 0;
}

bool Board::check() const {
    return isKingInCheck(whiteTurn);
}

void Board::applyMove(const Move& m) {
    switch (m.moveType) {
        case Move::Quiet:          updateByMove<Move::Quiet>(m);          break;
        case Move::Capture:        updateByMove<Move::Capture>(m);        break;
        case Move::EnPassant:      updateByMove<Move::EnPassant>(m);      break;
        case Move::Castle:         updateByMove<Move::Castle>(m);         break;
        case Move::Promote:        updateByMove<Move::Promote>(m);        break;
        case Move::PromoteCapture: updateByMove<Move::PromoteCapture>(m); break;
    }
}

bool Board::checkMate() {
    return check() && MoveGenerator::generateLegalMoves(*this).count == 0;
}

bool Board::staleMate() {
    return !check() && MoveGenerator::generateLegalMoves(*this).count == 0;
}

// ---------------------------------------------------------------------------
// Move application — specialisations
// ---------------------------------------------------------------------------

// Revoke the castle right associated with a rook starting square.
// Only the four starting squares (h1,a1,h8,a8) matter; any other square is a no-op.
static void revokeRookRight(bool castleRights[4], uint64_t sq) {
    if (sq & 1ULL)          castleRights[1] = false; // h1 → white short
    if (sq & (1ULL << 7))   castleRights[0] = false; // a1 → white long
    if (sq & (1ULL << 56))  castleRights[3] = false; // h8 → black short
    if (sq & (1ULL << 63))  castleRights[2] = false; // a8 → black long
}

// --- Quiet ---
template<>
void Board::updateByMove<Move::Quiet>(Move move) {
    savePrevState(move, *this);

    int oldRights = (castleRights[0] << 0)                                                                                         
                | (castleRights[1] << 1)                                                                          
                | (castleRights[2] << 2)                                                                                         
                | (castleRights[3] << 3); 

    int oldEnPassantSquare = enPassantSquare;

    auto [from, to, fromTo, colorIdx, pieceIdx] = moveCharacteristics(move);
    pieceBB[pieceIdx] ^= fromTo;
    pieceBB[colorIdx] ^= fromTo;
    occupiedBB        ^= fromTo;
    emptyBB           ^= fromTo;

    halfMoveClock = (move.piece == Move::Pawn) ? 0 : halfMoveClock + 1;


    enPassantSquare = -1;
    if (move.piece == Move::Pawn) {
        int delta = move.to - move.from;
        //check if there is an enemy pawn in to+1 or to-1
        int enemyColorIdx    = 7 - colorIdx;
        uint64_t enemyPawns = pieceBB[enemyColorIdx+1];
        bool enemyPawnPresence = enemyPawns & (
            (1ULL << (move.to+1))
            |(1ULL << (move.to-1)));

        if ((delta == 16 || delta == -16) && enemyPawnPresence){
            enPassantSquare = (move.from + move.to) / 2;
            // cout << "Setting epsquare: " << dec << enPassantSquare << endl;
        }
    }

    if (move.piece == Move::King) {
        castleRights[2 * move.color]     = false;
        castleRights[2 * move.color + 1] = false;
    } else if (move.piece == Move::Rook) {
        revokeRookRight(castleRights, from);
    }

    if (move.color == Move::Black) fullMoveNumber++;
    whiteTurn = !whiteTurn;
    moveHistory.push_back(move);

    int newRights = (castleRights[0] << 0)                                                                                         
                | (castleRights[1] << 1)                                                                          
                | (castleRights[2] << 2)                                                                                         
                | (castleRights[3] << 3); 
    
    hash ^= Zobrist::movePiece(pieceIdx, move.from, move.to);
    hash ^= Zobrist::castleRightsDelta(oldRights, newRights);
    hash ^= Zobrist::epFileDelta(oldEnPassantSquare, enPassantSquare);
    hash ^= Zobrist::switchSide();
}

// --- Capture ---
template<>
void Board::updateByMove<Move::Capture>(Move move) {
    savePrevState(move, *this);

    int oldRights = (castleRights[0] << 0)                                                                                         
            | (castleRights[1] << 1)                                                                          
            | (castleRights[2] << 2)                                                                                         
            | (castleRights[3] << 3);  

    int oldEnPassantSquare = enPassantSquare;

    auto [from, to, fromTo, colorIdx, pieceIdx] = moveCharacteristics(move);
    int enemyColorIdx    = 7 - colorIdx;
    int capturedPieceIdx = enemyColorIdx + move.cPiece + 1;

    pieceBB[pieceIdx]         ^= fromTo;
    pieceBB[colorIdx]         ^= fromTo;
    pieceBB[capturedPieceIdx] ^= to;
    pieceBB[enemyColorIdx]    ^= to;
    occupiedBB = pieceBB[0] | pieceBB[7];
    emptyBB    = ~occupiedBB;

    halfMoveClock   = 0;
    enPassantSquare = -1;

    if (move.cPiece == Move::Rook)   revokeRookRight(castleRights, to);
    if (move.piece == Move::King) {
        castleRights[2 * move.color]     = false;
        castleRights[2 * move.color + 1] = false;
    } else if (move.piece == Move::Rook) {
        revokeRookRight(castleRights, from);
    }

    if (move.color == Move::Black) fullMoveNumber++;
    whiteTurn = !whiteTurn;
    moveHistory.push_back(move);

    int newRights = (castleRights[0] << 0)                                                                                         
            | (castleRights[1] << 1)                                                                          
            | (castleRights[2] << 2)                                                                                         
            | (castleRights[3] << 3); 
    
    hash ^= Zobrist::movePiece(pieceIdx, move.from, move.to);
    hash ^= Zobrist::removePiece(capturedPieceIdx, move.to);
    hash ^= Zobrist::castleRightsDelta(oldRights, newRights);
    hash ^= Zobrist::epFileDelta(oldEnPassantSquare, enPassantSquare);
    hash ^= Zobrist::switchSide();
}

// --- En passant ---
// 'to' is the ep-target square (behind the captured pawn).
// The captured pawn sits one rank behind 'to' for the capturing side:
//   white captures: captured pawn at to-8  (to is on rank 6, pawn on rank 5)
//   black captures: captured pawn at to+8  (to is on rank 3, pawn on rank 4)
template<>
void Board::updateByMove<Move::EnPassant>(Move move) {
    savePrevState(move, *this);


    int oldEnPassantSquare = enPassantSquare;

    auto [from, to, fromTo, colorIdx, pieceIdx] = moveCharacteristics(move);
    int enemyColorIdx = 7 - colorIdx;
    int capturedPawnIdx = enemyColorIdx + Move::Pawn + 1;
    int capturedBit = move.to + (move.color == Move::White ? -8 : 8);
    uint64_t capturedBB = 1ULL << capturedBit;

    pieceBB[pieceIdx]       ^= fromTo;    // move capturing pawn
    pieceBB[colorIdx]       ^= fromTo;
    pieceBB[capturedPawnIdx] ^= capturedBB; // remove captured pawn
    pieceBB[enemyColorIdx]   ^= capturedBB;
    occupiedBB = pieceBB[0] | pieceBB[7];
    emptyBB    = ~occupiedBB;

    halfMoveClock   = 0;
    enPassantSquare = -1;

    if (move.color == Move::Black) fullMoveNumber++;
    whiteTurn = !whiteTurn;
    moveHistory.push_back(move);

    hash ^= Zobrist::movePiece(pieceIdx, move.from, move.to);
    hash ^= Zobrist::removePiece(capturedPawnIdx, capturedBit);
    hash ^= Zobrist::epFileDelta(oldEnPassantSquare, enPassantSquare);
    hash ^= Zobrist::switchSide();
}

// --- Promotion ---
template<>
void Board::updateByMove<Move::Promote>(Move move) {
    savePrevState(move, *this);

    int oldEnPassantSquare = enPassantSquare;

    auto [from, to, fromTo, colorIdx, pieceIdx] = moveCharacteristics(move);
    int newPieceIdx = colorIdx + move.newPiece + 1;

    pieceBB[pieceIdx]    &= ~from;   // remove pawn from source
    pieceBB[newPieceIdx] |= to;      // place promoted piece at dest
    pieceBB[colorIdx]    ^= fromTo;
    occupiedBB = pieceBB[0] | pieceBB[7];
    emptyBB    = ~occupiedBB;

    halfMoveClock   = 0;
    enPassantSquare = -1;

    if (move.color == Move::Black) fullMoveNumber++;
    whiteTurn = !whiteTurn;
    moveHistory.push_back(move);

    hash ^= Zobrist::movePiece(pieceIdx, move.from, move.to);
    hash ^= Zobrist::removePiece(pieceIdx, move.to);
    hash ^= Zobrist::removePiece(newPieceIdx, move.to);
    hash ^= Zobrist::epFileDelta(oldEnPassantSquare, enPassantSquare);
    hash ^= Zobrist::switchSide();
}

// --- Promote-capture ---
template<>
void Board::updateByMove<Move::PromoteCapture>(Move move) {
    savePrevState(move, *this);

    int oldEnPassantSquare = enPassantSquare;

    auto [from, to, fromTo, colorIdx, pieceIdx] = moveCharacteristics(move);
    int newPieceIdx      = colorIdx + move.newPiece + 1;
    int enemyColorIdx    = 7 - colorIdx;
    int capturedPieceIdx = enemyColorIdx + move.cPiece + 1;

    pieceBB[capturedPieceIdx] ^= to;     // remove captured piece first
    pieceBB[enemyColorIdx]    ^= to;
    pieceBB[pieceIdx]         &= ~from;  // remove pawn
    pieceBB[newPieceIdx]      |= to;     // place promoted piece
    pieceBB[colorIdx]         ^= fromTo;
    occupiedBB = pieceBB[0] | pieceBB[7];
    emptyBB    = ~occupiedBB;

    halfMoveClock   = 0;
    enPassantSquare = -1;

    if (move.cPiece == Move::Rook) revokeRookRight(castleRights, to);

    if (move.color == Move::Black) fullMoveNumber++;
    whiteTurn = !whiteTurn;
    moveHistory.push_back(move);

    hash ^= Zobrist::movePiece(pieceIdx, move.from, move.to);
    hash ^= Zobrist::removePiece(capturedPieceIdx, move.to);
    hash ^= Zobrist::removePiece(pieceIdx, move.to);
    hash ^= Zobrist::removePiece(newPieceIdx, move.to);
    hash ^= Zobrist::epFileDelta(oldEnPassantSquare, enPassantSquare);
    hash ^= Zobrist::switchSide();
}

// --- Castle ---
// Square indices (h1=bit0):
//   White O-O:   king 3→1, rook 0→2
//   White O-O-O: king 3→5, rook 7→4
//   Black O-O:   king 59→57, rook 56→58
//   Black O-O-O: king 59→61, rook 63→60
template<>
void Board::updateByMove<Move::Castle>(Move move) {
    savePrevState(move, *this);

    int oldEnPassantSquare = enPassantSquare;

    int oldRights = (castleRights[0] << 0)                                                                                         
            | (castleRights[1] << 1)                                                                          
            | (castleRights[2] << 2)                                                                                         
            | (castleRights[3] << 3);      

    int colorIdx = 7 * move.color;
    int kingIdx  = colorIdx + Move::King + 1;
    int rookIdx  = colorIdx + Move::Rook + 1;

    int kingFrom, kingTo, rookFrom, rookTo;
    if (move.color == Move::White) {
        kingFrom = 3;
        if (move.rookSide == Move::Kingside) { kingTo = 1; rookFrom = 0; rookTo = 2; }
        else                                 { kingTo = 5; rookFrom = 7; rookTo = 4; }
    } else {
        kingFrom = 59;
        if (move.rookSide == Move::Kingside) { kingTo = 57; rookFrom = 56; rookTo = 58; }
        else                                 { kingTo = 61; rookFrom = 63; rookTo = 60; }
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

    int newRights = (castleRights[0] << 0)                                                                                         
        | (castleRights[1] << 1)                                                                          
        | (castleRights[2] << 2)                                                                                         
        | (castleRights[3] << 3); 

    hash ^= Zobrist::movePiece(kingIdx, kingFrom, kingTo);
    hash ^= Zobrist::movePiece(rookIdx, rookFrom, rookTo);
    hash ^= Zobrist::castleRightsDelta(oldRights, newRights);
    hash ^= Zobrist::epFileDelta(oldEnPassantSquare, enPassantSquare);
    hash ^= Zobrist::switchSide();
}

// ---------------------------------------------------------------------------
// Unmake move
// ---------------------------------------------------------------------------
//TODO add reverse zobrist steps when unmaking a move
void Board::unmakeMove() {
    if (moveHistory.empty()) return;

    Move move = moveHistory.back();
    moveHistory.pop_back();

    int oldRights = (castleRights[0] << 0)                                                                                         
            | (castleRights[1] << 1)                                                                          
            | (castleRights[2] << 2)                                                                                         
            | (castleRights[3] << 3); 
    int oldEPSquare = enPassantSquare;

    


    // Restore metadata
    whiteTurn       = !whiteTurn;
    enPassantSquare = move.prevEnPassantSquare;
    halfMoveClock   = move.prevHalfMoveClock;


    castleRights[0] = move.prevCastleRights[0];
    castleRights[1] = move.prevCastleRights[1];
    castleRights[2] = move.prevCastleRights[2];
    castleRights[3] = move.prevCastleRights[3];
    int newRights = (castleRights[0] << 0)                                                                                         
        | (castleRights[1] << 1)                                                                          
        | (castleRights[2] << 2)                                                                                         
        | (castleRights[3] << 3); 
    if (move.color == Move::Black) fullMoveNumber--;

    auto [from, to, fromTo, colorIdx, pieceIdx] = moveCharacteristics(move);

    switch (move.moveType) {
        case Move::Quiet:
            pieceBB[pieceIdx] ^= fromTo;
            pieceBB[colorIdx] ^= fromTo;
            occupiedBB        ^= fromTo;
            emptyBB           ^= fromTo;

            hash ^= Zobrist::movePiece(pieceIdx, move.from, move.to);
            break;

        case Move::Capture: {
            int enemyColorIdx    = 7 - colorIdx;
            int capturedPieceIdx = enemyColorIdx + move.cPiece + 1;
            pieceBB[pieceIdx]         ^= fromTo;
            pieceBB[colorIdx]         ^= fromTo;
            pieceBB[capturedPieceIdx] ^= to;
            pieceBB[enemyColorIdx]    ^= to;
            occupiedBB = pieceBB[0] | pieceBB[7];
            emptyBB    = ~occupiedBB;

            hash ^= Zobrist::movePiece(pieceIdx, move.from, move.to);
            hash ^= Zobrist::removePiece(capturedPieceIdx, move.to);
            break;
        }

        case Move::EnPassant: {
            int enemyColorIdx   = 7 - colorIdx;
            int capturedPawnIdx = enemyColorIdx + Move::Pawn + 1;
            int capturedBit     = move.to + (move.color == Move::White ? -8 : 8);
            uint64_t capturedBB = 1ULL << capturedBit;
            pieceBB[pieceIdx]        ^= fromTo;
            pieceBB[colorIdx]        ^= fromTo;
            pieceBB[capturedPawnIdx] ^= capturedBB;
            pieceBB[enemyColorIdx]   ^= capturedBB;
            occupiedBB = pieceBB[0] | pieceBB[7];
            emptyBB    = ~occupiedBB;

            hash ^= Zobrist::movePiece(pieceIdx, move.from, move.to);
            hash ^= Zobrist::removePiece(capturedPawnIdx, capturedBit);
            break;
        }

        case Move::Promote: {
            int newPieceIdx = colorIdx + move.newPiece + 1;
            pieceBB[newPieceIdx] &= ~to;
            pieceBB[pieceIdx]    |= from;
            pieceBB[colorIdx]    ^= fromTo;
            occupiedBB = pieceBB[0] | pieceBB[7];
            emptyBB    = ~occupiedBB;
            hash ^= Zobrist::movePiece(pieceIdx, move.from, move.to);
            hash ^= Zobrist::removePiece(pieceIdx, move.to);
            hash ^= Zobrist::removePiece(newPieceIdx, move.to);
            break;
        }

        case Move::PromoteCapture: {
            int newPieceIdx      = colorIdx + move.newPiece + 1;
            int enemyColorIdx    = 7 - colorIdx;
            int capturedPieceIdx = enemyColorIdx + move.cPiece + 1;
            pieceBB[newPieceIdx]      &= ~to;
            pieceBB[pieceIdx]         |= from;
            pieceBB[colorIdx]         ^= fromTo;
            pieceBB[capturedPieceIdx] ^= to;
            pieceBB[enemyColorIdx]    ^= to;
            occupiedBB = pieceBB[0] | pieceBB[7];
            emptyBB    = ~occupiedBB;

            hash ^= Zobrist::movePiece(pieceIdx, move.from, move.to);
            hash ^= Zobrist::removePiece(capturedPieceIdx, move.to);
            hash ^= Zobrist::removePiece(pieceIdx, move.to);
            hash ^= Zobrist::removePiece(newPieceIdx, move.to);
            break;
        }

        case Move::Castle: {
            int kingIdx = colorIdx + Move::King + 1;
            int rookIdx = colorIdx + Move::Rook + 1;
            int kingFrom, kingTo, rookFrom, rookTo;
            if (move.color == Move::White) {
                kingFrom = 3;
                if (move.rookSide == Move::Kingside) { kingTo = 1; rookFrom = 0; rookTo = 2; }
                else                                 { kingTo = 5; rookFrom = 7; rookTo = 4; }
            } else {
                kingFrom = 59;
                if (move.rookSide == Move::Kingside) { kingTo = 57; rookFrom = 56; rookTo = 58; }
                else                                 { kingTo = 61; rookFrom = 63; rookTo = 60; }
            }
            uint64_t kingFromTo = (1ULL << kingFrom) | (1ULL << kingTo);
            uint64_t rookFromTo = (1ULL << rookFrom) | (1ULL << rookTo);
            uint64_t allFromTo  = kingFromTo | rookFromTo;
            pieceBB[kingIdx]  ^= kingFromTo;
            pieceBB[rookIdx]  ^= rookFromTo;
            pieceBB[colorIdx] ^= allFromTo;
            occupiedBB        ^= allFromTo;
            emptyBB           ^= allFromTo;

            hash ^= Zobrist::movePiece(kingIdx, kingFrom, kingTo);
            hash ^= Zobrist::movePiece(rookIdx, rookFrom, rookTo);
            break;
        }
    }
    hash ^= Zobrist::castleRightsDelta(oldRights, newRights);
    hash ^= Zobrist::epFileDelta(oldEPSquare, enPassantSquare);
    hash ^= Zobrist::switchSide();
}
