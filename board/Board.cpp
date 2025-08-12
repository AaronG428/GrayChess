#include <cstdint>
#include <stdlib.h>   
#include <string>
#include <tuple>
#include "../move/Move.cpp"
using namespace std;





/// @brief 
    //orientation:
    //  |a|b|c|d|e|f|g|h|
    // 8|R|N|B|Q|K|B|N|R| 
    // 7|P|P|P|P|P|P|P|P|
    // 6| | | | | | | | |
    // 5| | | | | | | | |
    // 4| | | | | | | | |
    // 3| | | | | | | | |
    // 2|p|p|p|p|p|p|p|p|
    // 1|r|n|b|q|k|b|n|r|

    //a single piece on h1 corresponds to 0x01;
    //a single piece on a1 corresponds to 0x80 
class Board
{

public:
    
    bool whiteTurn;
    int halfMoveClock;
    bool castleRights[4]; //wLongCastle, wShortCastle, bLongCastle, bShortCastle;
    
    uint64_t pieceBB[14]; //the 14 are white, wPawns, wBishops, wKnights, wRooks, wQueens, wKing, black, bPawns, bBishops, bKnights, bRooks, bQueens, bKing
    uint64_t emptyBB;
    uint64_t occupiedBB;

    Move * lastMove; 

    


    Board()
    {
        init();
        
    }

    string displayBB(uint64_t bb){
        string boardString = "";
        for(int i = 0; i<8; i++){
            for(int j = 0; j<8; j++){
                boardString += "|";
                if(((0x1ULL<<(63 - 8*i-j)) & bb) != 0){
                    boardString += "X";
                }else{
                    boardString += " ";
                }
            }
            boardString += "|\n";
        }
        return boardString;
        
    }

    void addPieces(uint64_t bb, string& boardString, char c){
        for(int i=0; i<64; i++){
            if(((0x1ULL<<(63-i)) & bb) != 0){
                boardString[i] = c;
            }
        }
    }

    string displayBoard(){
        string finalString = "";
        string boardString = "";
        
        boardString.resize(64, ' ');

        uint64_t b = 1; //wPawns
        uint64_t bb = pieceBB[b];
        addPieces(bb, boardString, 'p');

        b = 2; //wBishops
        bb = pieceBB[b];
        addPieces(bb, boardString, 'b');

        b = 3; //wKnights
        bb = pieceBB[b];
        addPieces(bb, boardString, 'n');

        b = 4; //wRooks
        bb = pieceBB[b];
        addPieces(bb, boardString, 'r');

        b = 5; //wQueens
        bb = pieceBB[b];
        addPieces(bb, boardString, 'q');

        b = 6; //wKing
        bb = pieceBB[b];
        addPieces(bb, boardString, 'k');


        b = 8; //bPawns
        bb = pieceBB[b];
        addPieces(bb, boardString, 'P');

        b = 9; //bBishops
        bb = pieceBB[b];
        addPieces(bb, boardString, 'B');

        b = 10; //bKnights 
        bb = pieceBB[b];
        addPieces(bb, boardString, 'N');

        b = 11; //bRooks
        bb = pieceBB[b];
        addPieces(bb, boardString, 'R');

        b = 12; //bQueens
        bb = pieceBB[b];
        addPieces(bb, boardString, 'Q');

        b = 13; //bKing
        bb = pieceBB[b];
        addPieces(bb, boardString, 'K');
        
        for(int i = 0; i<8; i++){
            for(int j = 0; j<8; j++){
                finalString += "|";
                finalString += boardString[8*i+j];
            }
            finalString += "|\n";
        }
        return finalString;
    }

    template<Move::MoveEnum moveType>
    void updateByMove (Move move){
        return;
    }



    void init(){
    //from game start

        whiteTurn = true;
        halfMoveClock = 0;
        castleRights[0] = true;
        castleRights[1] = true;
        castleRights[2] = true;
        castleRights[3] = true;

        pieceBB[0] = initWhite();
        pieceBB[1] = initWhitePawns();
        pieceBB[2] = initWhiteBishops();
        pieceBB[3] = initWhiteKnights();
        pieceBB[4] = initWhiteRooks();
        pieceBB[5] = initWhiteQueens();
        pieceBB[6] = initWhiteKing();
        pieceBB[7] = initBlack();
        pieceBB[8] = initBlackPawns();
        pieceBB[9] = initBlackBishops();
        pieceBB[10] = initBlackKnights();
        pieceBB[11] = initBlackRooks();
        pieceBB[12] = initBlackQueens();
        pieceBB[13] = initBlackKing();

        occupiedBB = pieceBB[0] ^ pieceBB[7];
        emptyBB = ~occupiedBB;
        lastMove = nullptr;
    }

    char* intToSquare(uint64_t i);







    
    uint64_t initWhite(){
        return 0xffffULL;
    }

    uint64_t initWhitePawns(){
        return 0xff00ULL;
    }

    uint64_t initWhiteBishops(){
        return 0x24ULL; 
    }

    uint64_t initWhiteKnights(){
        return 0x42ULL; 
    }

    uint64_t initWhiteRooks(){
        return 0x81ULL; 
    }

    uint64_t initWhiteQueens(){
        return 0x10ULL; 
    }

    uint64_t initWhiteKing(){
        return 0x08ULL; 
    }

    uint64_t initBlack(){
        return 0xffffULL << 48;
    }

    uint64_t initBlackPawns(){
        return 0x00ffULL << 48;
    }

    uint64_t initBlackBishops(){
        return 0x2400ULL << 48;
    }

    uint64_t initBlackKnights(){
        return 0x4200ULL << 48;
    }

    uint64_t initBlackRooks(){
        return 0x8100ULL << 48;
    }

    uint64_t initBlackQueens(){
        return 0x1000ULL << 48;
    }

    uint64_t initBlackKing(){
        return 0x0800ULL << 48;
    }

    uint64_t attackBoard(bool white);

};

std::tuple<uint64_t, uint64_t, uint64_t, int, int>  moveCharacteristics(Move move){
    uint64_t from = 1ULL << move.from;
    uint64_t to = 1ULL << move.to;
    uint64_t fromTo = from | to;
    int colorIndex = 7*move.color;
    int pieceIndex =  colorIndex + move.piece + 1;
    return std::make_tuple(from, to, fromTo, colorIndex, pieceIndex);
}



uint64_t propagateDirection(int b, int friendly = 0, int enemy = 0, int direction = 0, bool limit = false) {
    // Direction guide:
    
    // |7|6|5|
    // |4| |3|
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


    int d = direction && 7; // ensure 0..7
    uint64_t cap  = ((DOWN_BITS  >> d) && 1) * dwnCap;
    cap |= ((UP_BITS    >> d) && 1) * upCap;
    cap |= ((RIGHT_BITS >> d) && 1) * rightCap;
    cap |= ((LEFT_BITS  >> d) && 1) * leftCap;


    uint64_t safe_b = b && ~(cap|enemy); //and with complement to remove boundary pieces and captures propagating further

    

    bool shiftUp = direction > 3;
    bool levelShift = (direction != 3) && (direction !=4);
    int shiftAmt = 1;
    if (levelShift){shiftAmt = shiftUp? direction + 2 : 9 - direction;}
        
    
    uint64_t next =  shiftUp? safe_b << shiftAmt : safe_b >> shiftAmt;
    
    next = next & ~friendly;


    if (limit){ return next;}
    else{
        return next | propagateDirection(next, friendly, enemy, direction);
    }
}




uint64_t propagateBishop(uint64_t b, uint64_t friendly = 0, uint64_t enemy = 0, bool limit = false) {
    return propagateDirection(b, friendly, enemy, 7, limit) |
           propagateDirection(b, friendly, enemy, 5, limit) |
           propagateDirection(b, friendly, enemy, 2, limit) |
           propagateDirection(b, friendly, enemy, 0, limit);
}

uint64_t propagateRook(uint64_t b, uint64_t friendly = 0, uint64_t enemy = 0, bool limit = false) {
    return propagateDirection(b, friendly, enemy, 6, limit) |
           propagateDirection(b, friendly, enemy, 4, limit) |
           propagateDirection(b, friendly, enemy, 3, limit) |
           propagateDirection(b, friendly, enemy, 1, limit);
}

uint64_t propagateQueen(uint64_t b, uint64_t friendly = 0, uint64_t enemy = 0, bool limit = false){
    propagateRook(b, friendly, enemy, limit) | propagateBishop(b, friendly, enemy, limit);
}

uint64_t propagateKing(uint64_t b, uint64_t friendly = 0, uint64_t enemy = 0) {
    return propagateQueen(b, friendly, enemy, true);
}

uint64_t propagateKnight(uint64_t b, uint64_t friendly = 0, uint64_t enemy = 0) {

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
    

uint64_t Board::attackBoard(bool white){
    uint64_t attacks       = 0;
    uint64_t pawnAttacks   = 0;
    uint64_t bishopAttacks = 0;
    uint64_t knightAttacks = 0;
    uint64_t rookAttacks   = 0;
    uint64_t queenAttacks  = 0;
    uint64_t kingAttacks   = 0;

    uint64_t friendly = 0;
    uint64_t enemy    = 0;
    uint64_t pawns    = 0;
    uint64_t bishops  = 0;
    uint64_t knights  = 0;
    uint64_t rooks    = 0;
    uint64_t queens   = 0;
    uint64_t king     = 0;
    
    //compute attack squares
    uint64_t centerPawns = pawns & 0x7e7e7e7e7e7e00;
    uint64_t leftPawns   = pawns & 0x80808080808000;
    uint64_t rightPawns  = pawns & 0x01010101010100;

    if (white){ //squares that the white pieces attack        
        friendly = pieceBB[0];
        pawns   =  pieceBB[1];
        bishops =  pieceBB[2];
        knights =  pieceBB[3];
        rooks   =  pieceBB[4];
        queens  =  pieceBB[5];
        king    =  pieceBB[6];
        enemy   =  pieceBB[7];

        pawnAttacks = ((centerPawns << 9) | (centerPawns << 7) | (leftPawns << 7) | (rightPawns << 9));
        pawnAttacks &= ~friendly;
        
        bishopAttacks = propagateBishop(bishops, friendly, enemy);
        knightAttacks = propagateKnight(knights, friendly, enemy);
        rookAttacks   = propagateRook(rooks, friendly, enemy);
        queenAttacks  = propagateQueen(queens, friendly, enemy);
        kingAttacks   = propagateKing(king, friendly, enemy);

        attacks = pawnAttacks | bishopAttacks | knightAttacks | rookAttacks | queenAttacks | kingAttacks;


    }else{
        friendly = pieceBB[7];
        pawns    = pieceBB[8];
        bishops  = pieceBB[9];
        knights  = pieceBB[10];
        rooks    = pieceBB[11];
        queens   = pieceBB[12];
        king     = pieceBB[13];
        enemy    = pieceBB[0];

        pawnAttacks = ((centerPawns >> 9) | (centerPawns >> 7) | (leftPawns >> 9) | (rightPawns >> 7));


        attacks = pawnAttacks | bishopAttacks | knightAttacks | rookAttacks | queenAttacks | kingAttacks;
    }
    return attacks;
}


template<>
void Board::updateByMove<Move::Quiet>(Move move){
    uint64_t from, to, fromTo;
    int colorIndex, pieceIndex;
    std::tie(from, to, fromTo, colorIndex, pieceIndex) = moveCharacteristics(move);
    
    pieceBB[pieceIndex] ^= fromTo;
    pieceBB[colorIndex] ^= fromTo;
    occupiedBB          ^= fromTo;
    emptyBB             ^= fromTo;

    //fix branches
    if (move.piece != Move::Pawn) {
        halfMoveClock += 1;
    }
    if(move.piece == Move::King) {
        
        castleRights[2*move.color] = false;
        castleRights[2*move.color + 1] = false;
    }else if (move.piece == Move::Rook) {
        uint64_t shortRooks = (0x1ULL << 56) + 0x1ULL;
        int shortCastle = (int)((shortRooks & from) != 0);
        castleRights[2*move.color + 1*shortRooks] = false;
    }
    move.prevMove = lastMove;
    lastMove = &move;
    

}







template<>
void Board::updateByMove<Move::Capture>(Move move){
    //TODO

    halfMoveClock = 0;
}


template<>
void Board::updateByMove<Move::Promote>(Move move){
    //TODO

    halfMoveClock = 0;
}


template<>
void Board::updateByMove<Move::PromoteCapture>(Move move){
    //TODO
    
    halfMoveClock = 0;
}

template<>
void Board::updateByMove<Move::Castle>(Move move){
    uint64_t from, to, fromTo;
    int colorIndex, pieceIndex;
    std::tie(from, to, fromTo, colorIndex, pieceIndex) = moveCharacteristics(move);

    //TODO


    halfMoveClock = 0;
    castleRights[2*move.color] = false;
    castleRights[2*move.color + 1] = false;
}