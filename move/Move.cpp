#include <cstdint>
#include <sstream>
#include <string>

using namespace std;

class Move
{
public:
    enum PieceEnum{
        Pawn,
        Bishop,
        Knight,
        Rook,
        Queen,
        King
    };

    enum ColorEnum{
        White,
        Black
    };

    enum MoveEnum{
        Quiet,
        Castle,
        Capture,
        Promote,
        PromoteCapture
    };

    enum RookSide{
        Kingside,
        Queenside
    };
    
    MoveEnum moveType;

    int from;
    int to;

    PieceEnum piece;
    ColorEnum color;

    PieceEnum cPiece; //Captured Piece
    ColorEnum cColor; //Captured Color


    RookSide rookSide;

    PieceEnum newPiece; //for promotion

    Move* prevMove; //Move History


    static std::string toString(const Move& move) {
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

    static std::string bitboardPositionToNotation(int position) {
        char file = 'a' + (7-(position % 8));
        char rank = '1' + (position / 8);
        return std::string(1, file) + std::string(1, rank);
    }

    static std::string pieceNotiation(PieceEnum piece) {
        switch(piece) {
            case Bishop: return "B";
            case Knight: return "N";
            case Rook: return "R";
            case Queen: return "Q";
            case King: return "K";
            default: return "";
        }
    }


    static uint64_t Move::propagateDirection(uint64_t b, uint64_t friendly = 0, uint64_t enemy = 0, uint64_t direction = 0, bool limit = false);
    static uint64_t Move::propagateBishop(uint64_t b, uint64_t friendly = 0, uint64_t enemy = 0, bool limit = false);
    static uint64_t Move::propagateRook(uint64_t b, uint64_t friendly = 0, uint64_t enemy = 0, bool limit = false);
    static uint64_t Move::propagateQueen(uint64_t b, uint64_t friendly = 0, uint64_t enemy = 0, bool limit = false);
    static uint64_t Move::propagateKing(uint64_t b, uint64_t friendly = 0, uint64_t enemy = 0);
    static uint64_t Move::propagateKnight(uint64_t b, uint64_t friendly = 0, uint64_t enemy = 0);

};


uint64_t Move::propagateDirection(uint64_t b, uint64_t friendly = 0, uint64_t enemy = 0, uint64_t direction = 0, bool limit = false) {
    // Direction guide:
    
    // |7|6|5|
    // |4|X|3|
    // |2|1|0|


    if (b == 0) {
        return 0;
    }
    // # base masks
    uint64_t dwnCap   = 0xff;
    uint64_t upCap    = dwnCap << 56;
    uint64_t rightCap = 0x0101010101010101;
    uint64_t leftCap  = rightCap << 7;

    // # For directions 0..7, set a bit if that direction includes the component
    uint64_t DOWN_BITS  = 0b00000111; //  # dirs {0,1,2}
    uint64_t UP_BITS    = 0b11100000; //  # dirs {5,6,7}
    uint64_t RIGHT_BITS = 0b00101001; //  # dirs {0,3,5}
    uint64_t LEFT_BITS  = 0b10010100; //  # dirs {2,4,7}


    int d = direction & 7; // ensure 0..7
    uint64_t cap  = ((DOWN_BITS  >> d) & 1) * dwnCap;
    cap |= ((UP_BITS    >> d) & 1) * upCap;
    cap |= ((RIGHT_BITS >> d) & 1) * rightCap;
    cap |= ((LEFT_BITS  >> d) & 1) * leftCap;


    uint64_t safe_b = b & ~(cap|enemy); //and with complement to remove boundary pieces and captures propagating further

    

    bool shiftUp = direction > 3;
    bool levelShift = (direction != 3) & (direction !=4);
    int shiftAmt = 1;
    if (levelShift){shiftAmt = shiftUp? direction + 2 : 9 - direction;}
        
    
    uint64_t next =  shiftUp? safe_b << shiftAmt : safe_b >> shiftAmt;

    // cout << "Safe_b: 0x" << std::hex << safe_b << endl;
    next = next & ~friendly;
    // cout << "Next: 0x" << std::hex << next << endl;

    if (limit){ return next;}
    else{
        return next | propagateDirection(next, friendly, enemy, direction);
    }
}

uint64_t Move::propagateBishop(uint64_t b, uint64_t friendly = 0, uint64_t enemy = 0, bool limit = false) {
    return propagateDirection(b, friendly, enemy, 7, limit) |
           propagateDirection(b, friendly, enemy, 5, limit) |
           propagateDirection(b, friendly, enemy, 2, limit) |
           propagateDirection(b, friendly, enemy, 0, limit);
}

uint64_t Move::propagateRook(uint64_t b, uint64_t friendly = 0, uint64_t enemy = 0, bool limit = false) {
    return propagateDirection(b, friendly, enemy, 6, limit) |
           propagateDirection(b, friendly, enemy, 4, limit) |
           propagateDirection(b, friendly, enemy, 3, limit) |
           propagateDirection(b, friendly, enemy, 1, limit);
}

uint64_t Move::propagateQueen(uint64_t b, uint64_t friendly = 0, uint64_t enemy = 0, bool limit = false){
    return propagateRook(b, friendly, enemy, limit) | propagateBishop(b, friendly, enemy, limit);
}

uint64_t Move::propagateKing(uint64_t b, uint64_t friendly = 0, uint64_t enemy = 0) {
    return propagateQueen(b, friendly, enemy, true);
}

uint64_t Move::propagateKnight(uint64_t b, uint64_t friendly = 0, uint64_t enemy = 0) {

    /*
    Knight movement guide
    | |7| |6| |
    |5| | | |4|
    | | |X| | |
    |3| | | |2|
    | |1| |0| |

    Shift amounts
    0: >> 17
    1: >> 15
    2: >> 10
    3: >> 6
    4: << 6
    5: << 10
    6: << 15
    7: << 17
    */


    uint64_t full_board = 0xffffffffffffffff;
    // # base masks
    uint64_t dwnCap   = 0xff;
    uint64_t upCap    = dwnCap << 56;
    uint64_t rightCap = 0x0101010101010101;
    uint64_t leftCap  = rightCap << 7;

    uint64_t twoDwnCap = dwnCap << 8;
    uint64_t twoUpCap  = upCap >> 8;
    uint64_t twoRightCap = rightCap << 1;
    uint64_t twoLeftCap = leftCap >> 1 ;

    

    // # For directions 0..7, set a bit if that direction includes the component
    uint64_t DOWN_BITS      = 0b00001111; // # dirs {0,1,2,3}
    uint64_t UP_BITS        = 0b11110000; // # dirs {4,5,6,7}
    uint64_t RIGHT_BITS     = 0b01010101; // # dirs {0,2,4,6}
    uint64_t LEFT_BITS      = 0b10101010; // # dirs {1,3,5,7}

    uint64_t TWO_DOWN_BITS  = 0b00000011; //  # dirs {0,1}
    uint64_t TWO_UP_BITS    = 0b11000000; //  # dirs {6,7}
    uint64_t TWO_RIGHT_BITS = 0b00010100; //  # dirs {2,4}
    uint64_t TWO_LEFT_BITS  = 0b00101000; //  # dirs {3,5}
    
    int shiftAmts[8] = {-17, -15, -10, -6, 6, 10, 15, 17};
    uint64_t next = 0;
    
    for (int d=0; d<8; d++){
        uint64_t cap  = ((DOWN_BITS      >> d) & 1) * dwnCap;
        cap |= ((UP_BITS        >> d) & 1) * upCap;
        cap |= ((RIGHT_BITS     >> d) & 1) * rightCap;
        cap |= ((LEFT_BITS      >> d) & 1) * leftCap;
        cap |= ((TWO_DOWN_BITS  >> d) & 1) * twoDwnCap;
        cap |= ((TWO_UP_BITS    >> d) & 1) * twoUpCap;
        cap |= ((TWO_RIGHT_BITS >> d) & 1) * twoRightCap;
        cap |= ((TWO_LEFT_BITS  >> d) & 1) * twoLeftCap;
        uint64_t safe_b = b & ~(cap);
        int s = shiftAmts[d];
        next |=  s < 0 ? safe_b >> -s : safe_b << s;
    }
    next &= full_board;
    next &= ~friendly;
    return next;
    
}