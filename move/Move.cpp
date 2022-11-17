#include <cstdint>

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
    
    MoveEnum moveType;

    uint64_t from;
    uint64_t to;

    PieceEnum piece;
    ColorEnum color;

    PieceEnum cPiece;
    ColorEnum cColor;

    uint64_t rFrom;
    uint64_t fTo;

    PieceEnum newPiece;
    

};