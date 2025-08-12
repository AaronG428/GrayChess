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
        char file = 'a' + (position % 8);
        char rank = '1' + (position / 8);
        return std::string(1, file) + std::string(1, rank);
    }

    static std::string notation(const Move& move) {
        std::ostringstream sb;
        switch(move.moveType){
            case Move::Castle:
                if (move.rookSide == Move::Kingside) {
                    sb << "O-O";
                } else {
                    sb << "O-O-O";
                }
                return sb.str();
                break;
            case Move::Quiet:
                break;
            //TODO
                
        }

    }

};