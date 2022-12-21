#include <cstdint>
#include <string>
#include <tuple>
#include "../move/Move.cpp"
using namespace std;





/// @brief 
    //orientation:
    //  |a|b|c|d|e|f|g|h|
    // 8|R|N|B|Q|K|B|N|R| 
    // 7|p|p|p|p|p|p|p|p|
    // 6| | | | | | | | |
    // 5| | | | | | | | |
    // 4| | | | | | | | |
    // 3| | | | | | | | |
    // 2|p|p|p|p|p|p|p|p|
    // 1|R|N|B|Q|K|B|N|R|

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
    }







    
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

};

std::tuple<uint64_t, uint64_t, uint64_t, int, int>  moveCharacteristics(Move move){
    uint64_t from = 1ULL << move.from;
    uint64_t to = 1ULL << move.to;
    uint64_t fromTo = from | to;
    int colorIndex = 7*move.color;
    int pieceIndex =  colorIndex + move.piece + 1;
    return std::make_tuple(from, to, fromTo, colorIndex, pieceIndex);
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