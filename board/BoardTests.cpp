#include <cstdint>
#include <iostream>
#include "Board.cpp"
using namespace std;


    // enum class Animal { dog, cat, bird  };
    // class Sound
    // {
    // public:
    //     template<Animal animal>
    //     static void getSound ();
    // };

    // template<>
    // void Sound::getSound<Animal::dog> ()
    // {
    //     // dog specific processing
    //     cout << "woof" << endl;
    // }

    // template<>
    // void Sound::getSound<Animal::cat> ()
    // {
    //     // cat specific processing
    //     cout << "mrow" << endl;
    // }

    // template<>
    // void Sound::getSound<Animal::bird> ()
    // {
    //     // bird specific processing
    //     cout << "scree" << endl;
    // }

// void testSandbox()
// {
//     Animal animal = Animal::dog;
//     Sound::getSound<Animal::dog>();
//     //Sound::getSound<animal>();

// }

void moveTests()
{
    //test standard moves from white
    
        //e4
    Move move;
    move.from = 11;
    move.to = move.from+16;
    move.piece = Move::Pawn;
    move.color = Move::White;
    move.moveType = Move::Quiet;

    const Move::MoveEnum moveType = move.moveType;

    Board* board = new Board();
    board->updateByMove<Move::Quiet>(move);
    // for (int i=0; i<14; i++){
    //     cout << board->displayBB(board->pieceBB[i]) << endl;
    // }
    // cout << board->displayBB(board->occupiedBB) << endl;
    // cout << board->displayBB(board->emptyBB) << endl;
    
    cout << board->displayBoard() << endl;
    //show white's attacks
    cout << "White's attacks:" << endl;
    cout << board->displayBB(board->attackBoard(true)) << endl;

        //e5
        {
        move.from = 51;
        move.to = move.from-16;
        move.piece = Move::Pawn;
        move.color = Move::Black;
        move.moveType = Move::Quiet;

        board->updateByMove<Move::Quiet>(move);
        cout << board->displayBoard() << endl;

        
        }

    //show black's attacks
    cout << "Black's attacks:" << endl;
    cout << board->displayBB(board->attackBoard(false)) << endl;
        //Nf3
        {
        move.from = 1;
        move.to = move.from+16+1;
        move.piece = Move::Knight;
        move.color = Move::White;
        move.moveType = Move::Quiet;

        board->updateByMove<Move::Quiet>(move);
        cout << board->displayBoard() << endl;
        }
    cout << "White's attacks:" << endl;
    cout << board->displayBB(board->attackBoard(true)) << endl;
        

    //test standard moves from black    

    //test castle rights

    //test captures

    //test promotes

    //test capture/promotes

}

void runTests()
{
    //testing playground
    // bool runSandbox = false;
    // if(runSandbox){
    //     testSandbox();
    //     return;
    // }
    //Dedicated tests
    moveTests();

    cout << (0x1ULL <<7) << endl;
    
    

    
}


int main()
{
    runTests();
    return 0;
}