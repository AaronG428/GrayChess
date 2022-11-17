#include <cstdint>
#include <iostream>
#include "Board.cpp"
using namespace std;

void testSandbox()
{
    Board* board = new Board();
    cout << board->pieceBB[13] << endl;

    cout << board->initBlackKing() << endl;

    for (int i=0; i<14; i++){
        cout << board->displayBB(board->pieceBB[i]) << endl;
    }
    cout << board->displayBB(board->occupiedBB) << endl;
    cout << board->displayBB(board->emptyBB) << endl;
}

void moveTests()
{
    //test standard moves from white
    
    //e4
    Move move;
    move.from = 0x1ULL << 11;
    move.to = move.from << 16;
    move.piece = Move::Pawn;
    move.color = Move::White;
    move.moveType = Move::Quiet;

    Board* board = new Board();
    board->updateByMove<move.moveType>(move);
    for (int i=0; i<14; i++){
        cout << board->displayBB(board->pieceBB[i]) << endl;
    }
    cout << board->displayBB(board->occupiedBB) << endl;
    cout << board->displayBB(board->emptyBB) << endl;

    //test standard moves from black    

    //test castle rights

    //test captures

    //test promotes

    //test capture/promotes

}

void runTests()
{
    //testing playground
    bool runSandbox = false;
    if(runSandbox){
        testSandbox();
        return;
    }
    //Dedicated tests
    moveTests();
    
    

    
}


int main()
{
    runTests();
    return 0;
}