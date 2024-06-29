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
        
        //add all pawn moves

        


        return;
    }

    static bool checkMate(Board board){
        //TODO
        return false;
    }

    static bool staleMate(Board board){
        //TODO
        return false;
    }

};