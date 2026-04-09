#pragma once
#include "../board/Board.h"
#include "../engine/Search.h"
#include "../engine/TranspositionTable.h"
#include "../move/MoveGenerator.h"
#include <sstream>
#include <string>
#include <thread>

class UCI {
public:
    UCI();
    void loop();

    // Exposed for unit testing.
    const Board& board() const { return board_; }
    void         handlePosition(std::istringstream& ss);
    Move         parseMoveUCI(const std::string& uci);
    static int         squareFromUCI(const std::string& s);
    static std::string moveToUCI(const Move& m);

private:
    Board                    board_;
    TranspositionTable       tt_;
    Search                   search_;
    std::thread              searchThread_;
    std::vector<uint64_t>    gameHashes_;

    void handleUci();
    void handleIsReady();
    void handleUciNewGame();
    void handleSetOption(std::istringstream& ss);
    void handleGo(std::istringstream& ss);
    void handleStop();
};
