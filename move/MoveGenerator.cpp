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
    static vector<Move> generateMoves(Board& board){
        //check if in endgame state
        if( board.checkMate() || board.staleMate() )
        {
            return vector<Move>();
        }

        if( board.check()){
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





};