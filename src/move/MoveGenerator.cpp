#include "MoveGenerator.h"
#include "../board/Board.h"
#include "../move/AttackTables.h"
#include "../utils/Bits.h"

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

// Return the piece type belonging to `enemyColorIdx` that occupies square `sq`.
static Move::PieceEnum pieceTypeAt(const Board& board, int sq, int enemyColorIdx) {
    uint64_t bit = 1ULL << sq;
    for (int p = 0; p < 6; p++) {
        if (board.pieceBB[enemyColorIdx + 1 + p] & bit)
            return static_cast<Move::PieceEnum>(p);
    }
    return Move::Pawn; // unreachable if sq is genuinely occupied by the enemy
}

// Emit the four promotion variants for a pawn that reaches the back rank.
static void addPromos(MoveList& list, int from, int to,
                      Move::ColorEnum color, Move::ColorEnum enemy,
                      bool isCapture, Move::PieceEnum cPiece) {
    static const Move::PieceEnum PROMO_PIECES[] =
        { Move::Queen, Move::Rook, Move::Bishop, Move::Knight };

    for (Move::PieceEnum promo : PROMO_PIECES) {
        Move m;
        m.from     = from;
        m.to       = to;
        m.piece    = Move::Pawn;
        m.color    = color;
        m.newPiece = promo;
        m.moveType = isCapture ? Move::PromoteCapture : Move::Promote;
        if (isCapture) { m.cPiece = cPiece; m.cColor = enemy; }
        list.push(m);
    }
}

// ---------------------------------------------------------------------------
// Pawn moves
// ---------------------------------------------------------------------------
static void genPawnMoves(const Board& board, MoveList& list) {
    const bool white = board.whiteTurn;
    const int  colorIdx      = white ? 0 : 7;
    const int  enemyColorIdx = white ? 7 : 0;
    const Move::ColorEnum colorEnum = white ? Move::White : Move::Black;
    const Move::ColorEnum enemyEnum = white ? Move::Black : Move::White;
    const int  colorBit      = white ? 0 : 1;   // index into PAWN_ATTACKS[color]

    const uint64_t pawns = board.pieceBB[colorIdx + 1];
    const uint64_t enemy = board.pieceBB[enemyColorIdx];
    const uint64_t empty = board.emptyBB;

    uint64_t bb = pawns;
    while (bb) {
        const int from = lsb(bb); bb &= bb - 1;
        const int rank = from / 8;

        // Pawn is one step before the promotion rank:
        //   white: rank 7 (sq/8 == 6), push lands on rank 8 (sq/8 == 7)
        //   black: rank 2 (sq/8 == 1), push lands on rank 1 (sq/8 == 0)
        const bool isPromoRank  = white ? (rank == 6) : (rank == 1);
        const bool isStartRank  = white ? (rank == 1) : (rank == 6);
        const int  push         = white ? 8 : -8;

        // --- Single push ---
        const int to1 = from + push;
        if ((1ULL << to1) & empty) {
            if (isPromoRank) {
                addPromos(list, from, to1, colorEnum, enemyEnum, false, Move::Pawn);
            } else {
                Move m; m.moveType = Move::Quiet; m.from = from; m.to = to1;
                m.piece = Move::Pawn; m.color = colorEnum;
                list.push(m);
            }

            // --- Double push (only reachable if single-push square is empty) ---
            if (isStartRank) {
                const int to2 = from + 2 * push;
                if ((1ULL << to2) & empty) {
                    Move m; m.moveType = Move::Quiet; m.from = from; m.to = to2;
                    m.piece = Move::Pawn; m.color = colorEnum;
                    list.push(m);
                }
            }
        }

        // --- Pawn captures ---
        uint64_t attacks = AttackTables::PAWN_ATTACKS[colorBit][from] & enemy;
        while (attacks) {
            const int cap = lsb(attacks); attacks &= attacks - 1;
            const Move::PieceEnum cPiece = pieceTypeAt(board, cap, enemyColorIdx);
            if (isPromoRank) {
                addPromos(list, from, cap, colorEnum, enemyEnum, true, cPiece);
            } else {
                Move m; m.moveType = Move::Capture; m.from = from; m.to = cap;
                m.piece = Move::Pawn; m.color = colorEnum;
                m.cPiece = cPiece; m.cColor = enemyEnum;
                list.push(m);
            }
        }

        // --- En passant ---
        if (board.enPassantSquare != -1) {
            const uint64_t epBit = 1ULL << board.enPassantSquare;
            if (AttackTables::PAWN_ATTACKS[colorBit][from] & epBit) {
                Move m; m.moveType = Move::EnPassant; m.from = from;
                m.to = board.enPassantSquare;
                m.piece = Move::Pawn; m.color = colorEnum;
                list.push(m);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Knight moves
// ---------------------------------------------------------------------------
static void genKnightMoves(const Board& board, MoveList& list) {
    const bool white        = board.whiteTurn;
    const int  colorIdx     = white ? 0 : 7;
    const int  enemyColorIdx = white ? 7 : 0;
    const Move::ColorEnum colorEnum = white ? Move::White : Move::Black;
    const Move::ColorEnum enemyEnum = white ? Move::Black : Move::White;

    const uint64_t friendly = board.pieceBB[colorIdx];
    const uint64_t enemy    = board.pieceBB[enemyColorIdx];

    uint64_t bb = board.pieceBB[colorIdx + 3]; // knights
    while (bb) {
        const int from = lsb(bb); bb &= bb - 1;
        uint64_t targets = AttackTables::KNIGHT_ATTACKS[from] & ~friendly;

        uint64_t quiets = targets & ~enemy;
        while (quiets) {
            const int to = lsb(quiets); quiets &= quiets - 1;
            Move m; m.moveType = Move::Quiet; m.from = from; m.to = to;
            m.piece = Move::Knight; m.color = colorEnum;
            list.push(m);
        }

        uint64_t captures = targets & enemy;
        while (captures) {
            const int to = lsb(captures); captures &= captures - 1;
            Move m; m.moveType = Move::Capture; m.from = from; m.to = to;
            m.piece = Move::Knight; m.color = colorEnum;
            m.cPiece = pieceTypeAt(board, to, enemyColorIdx);
            m.cColor = enemyEnum;
            list.push(m);
        }
    }
}

// ---------------------------------------------------------------------------
// Sliding piece moves (bishop, rook, queen)
// ---------------------------------------------------------------------------
static void genSliderMoves(const Board& board, MoveList& list,
                           Move::PieceEnum piece, uint64_t pieces,
                           int colorIdx, int enemyColorIdx,
                           Move::ColorEnum colorEnum, Move::ColorEnum enemyEnum) {
    const uint64_t friendly = board.pieceBB[colorIdx];
    const uint64_t enemy    = board.pieceBB[enemyColorIdx];
    const uint64_t occ      = board.occupiedBB;

    uint64_t bb = pieces;
    while (bb) {
        const int from = lsb(bb); bb &= bb - 1;

        uint64_t targets;
        if      (piece == Move::Rook)   targets = AttackTables::rookAttacks  (from, occ);
        else if (piece == Move::Bishop) targets = AttackTables::bishopAttacks(from, occ);
        else                            targets = AttackTables::queenAttacks  (from, occ);
        targets &= ~friendly;

        uint64_t quiets = targets & ~enemy;
        while (quiets) {
            const int to = lsb(quiets); quiets &= quiets - 1;
            Move m; m.moveType = Move::Quiet; m.from = from; m.to = to;
            m.piece = piece; m.color = colorEnum;
            list.push(m);
        }

        uint64_t captures = targets & enemy;
        while (captures) {
            const int to = lsb(captures); captures &= captures - 1;
            Move m; m.moveType = Move::Capture; m.from = from; m.to = to;
            m.piece = piece; m.color = colorEnum;
            m.cPiece = pieceTypeAt(board, to, enemyColorIdx);
            m.cColor = enemyEnum;
            list.push(m);
        }
    }
}

// ---------------------------------------------------------------------------
// King moves (non-castle)
// ---------------------------------------------------------------------------
static void genKingMoves(const Board& board, MoveList& list) {
    const bool white        = board.whiteTurn;
    const int  colorIdx     = white ? 0 : 7;
    const int  enemyColorIdx = white ? 7 : 0;
    const Move::ColorEnum colorEnum = white ? Move::White : Move::Black;
    const Move::ColorEnum enemyEnum = white ? Move::Black : Move::White;

    const uint64_t friendly = board.pieceBB[colorIdx];
    const uint64_t enemy    = board.pieceBB[enemyColorIdx];

    uint64_t bb = board.pieceBB[colorIdx + 6]; // king
    while (bb) {
        const int from = lsb(bb); bb &= bb - 1;
        uint64_t targets = AttackTables::KING_ATTACKS[from] & ~friendly;

        uint64_t quiets = targets & ~enemy;
        while (quiets) {
            const int to = lsb(quiets); quiets &= quiets - 1;
            Move m; m.moveType = Move::Quiet; m.from = from; m.to = to;
            m.piece = Move::King; m.color = colorEnum;
            list.push(m);
        }

        uint64_t captures = targets & enemy;
        while (captures) {
            const int to = lsb(captures); captures &= captures - 1;
            Move m; m.moveType = Move::Capture; m.from = from; m.to = to;
            m.piece = Move::King; m.color = colorEnum;
            m.cPiece = pieceTypeAt(board, to, enemyColorIdx);
            m.cColor = enemyEnum;
            list.push(m);
        }
    }
}

// ---------------------------------------------------------------------------
// Castle moves
//
// Square indices (h1=bit0):
//   White O-O:   king e1(3)→g1(1), rook h1(0)→f1(2).  Empty: {1,2}. King path: {3,2,1}
//   White O-O-O: king e1(3)→c1(5), rook a1(7)→d1(4).  Empty: {4,5,6}. King path: {3,4,5}
//   Black O-O:   king e8(59)→g8(57), rook h8(56)→f8(58). Empty: {57,58}. King path: {59,58,57}
//   Black O-O-O: king e8(59)→c8(61), rook a8(63)→d8(60). Empty: {60,61,62}. King path: {59,60,61}
//
// The king must not start in, pass through, or land on an attacked square.
// (Phase 5 legality filter will catch post-move check, but the through-check
//  rule requires checking intermediate squares here.)
// ---------------------------------------------------------------------------
static void genCastleMoves(const Board& board, MoveList& list) {
    const bool white = board.whiteTurn;
    const Move::ColorEnum colorEnum = white ? Move::White : Move::Black;

    // castleRights[0]=wLong, [1]=wShort, [2]=bLong, [3]=bShort
    const int longIdx  = white ? 0 : 2; // queenside
    const int shortIdx = white ? 1 : 3; // kingside

    const int kingFrom = white ? 3 : 59;

    // Precompute squares the opponent attacks (needed for through-check test)
    const uint64_t attacked = board.attackBoard(!white);

    struct CastleSpec {
        bool      right;
        uint64_t  emptyMask; // squares that must be unoccupied
        uint64_t  kingPath;  // king origin + transit + destination (all 3 must be safe)
        int       kingTo;
        Move::RookSide side;
    };

    CastleSpec specs[2];
    if (white) {
        specs[0] = { board.castleRights[shortIdx],
                     (1ULL<<1)|(1ULL<<2),
                     (1ULL<<3)|(1ULL<<2)|(1ULL<<1),
                     1, Move::Kingside };
        specs[1] = { board.castleRights[longIdx],
                     (1ULL<<4)|(1ULL<<5)|(1ULL<<6),
                     (1ULL<<3)|(1ULL<<4)|(1ULL<<5),
                     5, Move::Queenside };
    } else {
        specs[0] = { board.castleRights[shortIdx],
                     (1ULL<<57)|(1ULL<<58),
                     (1ULL<<59)|(1ULL<<58)|(1ULL<<57),
                     57, Move::Kingside };
        specs[1] = { board.castleRights[longIdx],
                     (1ULL<<60)|(1ULL<<61)|(1ULL<<62),
                     (1ULL<<59)|(1ULL<<60)|(1ULL<<61),
                     61, Move::Queenside };
    }

    for (const CastleSpec& c : specs) {
        if (!c.right)                        continue; // right revoked
        if (c.emptyMask & board.occupiedBB)  continue; // path blocked
        if (c.kingPath  & attacked)          continue; // king in/through check
        Move m;
        m.moveType = Move::Castle;
        m.piece    = Move::King;
        m.color    = colorEnum;
        m.rookSide = c.side;
        m.from     = kingFrom;
        m.to       = c.kingTo;
        list.push(m);
    }
}

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------
MoveList MoveGenerator::generateMoves(const Board& board) {
    MoveList list;

    const bool white        = board.whiteTurn;
    const int  colorIdx     = white ? 0 : 7;
    const int  enemyColorIdx = white ? 7 : 0;
    const Move::ColorEnum colorEnum = white ? Move::White : Move::Black;
    const Move::ColorEnum enemyEnum = white ? Move::Black : Move::White;

    genPawnMoves  (board, list);
    genKnightMoves(board, list);
    genSliderMoves(board, list, Move::Bishop, board.pieceBB[colorIdx + 2],
                   colorIdx, enemyColorIdx, colorEnum, enemyEnum);
    genSliderMoves(board, list, Move::Rook,   board.pieceBB[colorIdx + 4],
                   colorIdx, enemyColorIdx, colorEnum, enemyEnum);
    genSliderMoves(board, list, Move::Queen,  board.pieceBB[colorIdx + 5],
                   colorIdx, enemyColorIdx, colorEnum, enemyEnum);
    genKingMoves  (board, list);
    genCastleMoves(board, list);

    return list;
}

MoveList MoveGenerator::generateCaptures(const Board& board) {
    MoveList list;

    const bool white        = board.whiteTurn;
    const int  colorIdx     = white ? 0 : 7;
    const int  enemyColorIdx = white ? 7 : 0;
    const Move::ColorEnum colorEnum = white ? Move::White : Move::Black;
    const Move::ColorEnum enemyEnum = white ? Move::Black : Move::White;
    const int  colorBit     = white ? 0 : 1;

    const uint64_t enemy = board.pieceBB[enemyColorIdx];
    const uint64_t occ   = board.occupiedBB;

    // Pawns: captures + en passant + promote-captures
    {
        uint64_t bb = board.pieceBB[colorIdx + 1];
        while (bb) {
            const int from = lsb(bb); bb &= bb - 1;
            const int rank = from / 8;
            const bool isPromoRank = white ? (rank == 6) : (rank == 1);

            uint64_t attacks = AttackTables::PAWN_ATTACKS[colorBit][from] & enemy;
            while (attacks) {
                const int cap = lsb(attacks); attacks &= attacks - 1;
                const Move::PieceEnum cPiece = pieceTypeAt(board, cap, enemyColorIdx);
                if (isPromoRank) {
                    addPromos(list, from, cap, colorEnum, enemyEnum, true, cPiece);
                } else {
                    Move m; m.moveType = Move::Capture; m.from = from; m.to = cap;
                    m.piece = Move::Pawn; m.color = colorEnum;
                    m.cPiece = cPiece; m.cColor = enemyEnum;
                    list.push(m);
                }
            }

            if (board.enPassantSquare != -1) {
                const uint64_t epBit = 1ULL << board.enPassantSquare;
                if (AttackTables::PAWN_ATTACKS[colorBit][from] & epBit) {
                    Move m; m.moveType = Move::EnPassant; m.from = from;
                    m.to = board.enPassantSquare;
                    m.piece = Move::Pawn; m.color = colorEnum;
                    list.push(m);
                }
            }
        }
    }

    // Knights
    {
        const uint64_t friendly = board.pieceBB[colorIdx];
        uint64_t bb = board.pieceBB[colorIdx + 3];
        while (bb) {
            const int from = lsb(bb); bb &= bb - 1;
            uint64_t captures = AttackTables::KNIGHT_ATTACKS[from] & enemy & ~friendly;
            while (captures) {
                const int to = lsb(captures); captures &= captures - 1;
                Move m; m.moveType = Move::Capture; m.from = from; m.to = to;
                m.piece = Move::Knight; m.color = colorEnum;
                m.cPiece = pieceTypeAt(board, to, enemyColorIdx); m.cColor = enemyEnum;
                list.push(m);
            }
        }
    }

    // Bishops
    {
        const uint64_t friendly = board.pieceBB[colorIdx];
        uint64_t bb = board.pieceBB[colorIdx + 2];
        while (bb) {
            const int from = lsb(bb); bb &= bb - 1;
            uint64_t captures = AttackTables::bishopAttacks(from, occ) & enemy & ~friendly;
            while (captures) {
                const int to = lsb(captures); captures &= captures - 1;
                Move m; m.moveType = Move::Capture; m.from = from; m.to = to;
                m.piece = Move::Bishop; m.color = colorEnum;
                m.cPiece = pieceTypeAt(board, to, enemyColorIdx); m.cColor = enemyEnum;
                list.push(m);
            }
        }
    }

    // Rooks
    {
        const uint64_t friendly = board.pieceBB[colorIdx];
        uint64_t bb = board.pieceBB[colorIdx + 4];
        while (bb) {
            const int from = lsb(bb); bb &= bb - 1;
            uint64_t captures = AttackTables::rookAttacks(from, occ) & enemy & ~friendly;
            while (captures) {
                const int to = lsb(captures); captures &= captures - 1;
                Move m; m.moveType = Move::Capture; m.from = from; m.to = to;
                m.piece = Move::Rook; m.color = colorEnum;
                m.cPiece = pieceTypeAt(board, to, enemyColorIdx); m.cColor = enemyEnum;
                list.push(m);
            }
        }
    }

    // Queens
    {
        const uint64_t friendly = board.pieceBB[colorIdx];
        uint64_t bb = board.pieceBB[colorIdx + 5];
        while (bb) {
            const int from = lsb(bb); bb &= bb - 1;
            uint64_t captures = AttackTables::queenAttacks(from, occ) & enemy & ~friendly;
            while (captures) {
                const int to = lsb(captures); captures &= captures - 1;
                Move m; m.moveType = Move::Capture; m.from = from; m.to = to;
                m.piece = Move::Queen; m.color = colorEnum;
                m.cPiece = pieceTypeAt(board, to, enemyColorIdx); m.cColor = enemyEnum;
                list.push(m);
            }
        }
    }

    // King captures
    {
        const uint64_t friendly = board.pieceBB[colorIdx];
        uint64_t bb = board.pieceBB[colorIdx + 6];
        while (bb) {
            const int from = lsb(bb); bb &= bb - 1;
            uint64_t captures = AttackTables::KING_ATTACKS[from] & enemy & ~friendly;
            while (captures) {
                const int to = lsb(captures); captures &= captures - 1;
                Move m; m.moveType = Move::Capture; m.from = from; m.to = to;
                m.piece = Move::King; m.color = colorEnum;
                m.cPiece = pieceTypeAt(board, to, enemyColorIdx); m.cColor = enemyEnum;
                list.push(m);
            }
        }
    }

    return list;
}

// ---------------------------------------------------------------------------
// Legal move generation (Phase 5)
// ---------------------------------------------------------------------------
MoveList MoveGenerator::generateLegalMoves(const Board& board) {
    MoveList pseudo = generateMoves(board);
    MoveList legal;
    for (int i = 0; i < pseudo.count; i++) {
        Board copy = board;
        copy.applyMove(pseudo.moves[i]);
        // After the move, whiteTurn has toggled — the side that just moved is !copy.whiteTurn
        if (!copy.isKingInCheck(!copy.whiteTurn))
            legal.push(pseudo.moves[i]);
    }
    return legal;
}
