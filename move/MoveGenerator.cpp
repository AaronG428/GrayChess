#include "Move.cpp"
#include "../board/Board.cpp"
#include <vector>

class Move;

class MoveGenerator
{
    public:

    MoveGenerator()
    {
        return;
    };

    //static generator function?
    static vector<Move> generateMoves(Board board){
        //check if in endgame state
        if( checkMate(board) || staleMate(board) )
        {
            return vector<Move>();
        }

        if( check(board)){
            //add all possible moves that do not leave the king in check
        }
        
        //add all pawn moves

        //add all knight moves

        //add all bishop moves

        //add all rook moves

        //add all queen moves

        //add all king moves
            //

        

        return;
    }

    static bool check(Board board){
        
        return false;
    }

    static bool checkMate(Board board){
        if (!check(board)){
            return false;
        }
        //TODO
        return false;
    }

    static bool staleMate(Board board){
        //TODO
        return false;
    }

};