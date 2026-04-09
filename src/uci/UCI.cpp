#include "UCI.h"
#include <chrono>
#include <iostream>
#include <sstream>

UCI::UCI() : tt_(1u << 23), search_(tt_) {
    board_.init();
    
}

// ── Public loop ─────────────────────────────────────────────────────────────

void UCI::loop() {
    std::ios::sync_with_stdio(false);
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream ss(line);
        std::string cmd;
        ss >> cmd;

        if      (cmd == "uci")        handleUci();
        else if (cmd == "isready")    handleIsReady();
        else if (cmd == "ucinewgame") handleUciNewGame();
        else if (cmd == "setoption")  handleSetOption(ss);
        else if (cmd == "position")   handlePosition(ss);
        else if (cmd == "go")         handleGo(ss);
        else if (cmd == "stop")       handleStop();
        else if (cmd == "quit")       { handleStop(); break; }
    }
    if (searchThread_.joinable()) searchThread_.join();
}

// ── UCI commands ─────────────────────────────────────────────────────────────

void UCI::handleUci() {
    std::cout << "id name GrayChess\n"
              << "id author GrayChess\n"
              << "option name MultiPV type spin default 1 min 1 max 10\n"
              << "uciok\n" << std::flush;
}

void UCI::handleSetOption(std::istringstream& ss) {
    std::string token, name, value;
    if (!(ss >> token) || token != "name") return;
    ss >> name;
    if (!(ss >> token) || token != "value") return;
    ss >> value;
    if (name == "MultiPV")
        search_.setMultiPV(std::stoi(value));
}

void UCI::handleIsReady() {
    std::cout << "readyok\n" << std::flush;
}

void UCI::handleUciNewGame() {
    if (searchThread_.joinable()) searchThread_.join();
    board_.init();
    tt_.clear();
    gameHashes_.clear();
}

void UCI::handlePosition(std::istringstream& ss) {
    if (searchThread_.joinable()) searchThread_.join();

    gameHashes_.clear();

    std::string token;
    ss >> token;

    bool foundMoves = false;

    if (token == "startpos") {
        board_.init();
        if (ss >> token && token == "moves")
            foundMoves = true;
    } else if (token == "fen") {
        std::string fen;
        while (ss >> token && token != "moves") {
            if (!fen.empty()) fen += ' ';
            fen += token;
        }
        board_.loadFEN(fen);
        if (token == "moves") foundMoves = true;
    }

    if (foundMoves) {
        while (ss >> token) {
            gameHashes_.push_back(board_.hash);
            board_.applyMove(parseMoveUCI(token));
        }
    }
}

void UCI::handleGo(std::istringstream& ss) {
    if (searchThread_.joinable()) searchThread_.join();

    std::string token;
    int  depth       = 100; // default: search deep until stopped
    int  wtime       = 0, btime = 0, winc = 0, binc = 0, movetime = 0;
    bool hasDepth    = false;
    bool hasMovetime = false;

    while (ss >> token) {
        if      (token == "infinite")  { /* depth stays at 100, no time limit */ }
        else if (token == "depth")    { ss >> depth;    hasDepth    = true; }
        else if (token == "movetime") { ss >> movetime; hasMovetime = true; }
        else if (token == "wtime")    { ss >> wtime; }
        else if (token == "btime")    { ss >> btime; }
        else if (token == "winc")     { ss >> winc; }
        else if (token == "binc")     { ss >> binc; }
    }

    search_.resetStop();
    search_.setGameHistory(gameHashes_);

    if (!hasDepth) {
        long allocatedMs = 0;
        if (hasMovetime) {
            allocatedMs = movetime;
        } else if (wtime > 0 || btime > 0) {
            int myTime = board_.whiteTurn ? wtime : btime;
            int myInc  = board_.whiteTurn ? winc  : binc;
            allocatedMs = std::max(50, myTime / 30 + myInc / 2);
        }
        if (allocatedMs > 0) search_.setTimeLimit(allocatedMs);
    }

    searchThread_ = std::thread([this, depth]() {
        auto cb = [this](int multipv, int d, int score,
                         const std::vector<Move>& pv) {
            long     ms    = search_.elapsedMs();
            uint64_t nodes = search_.nodes();
            uint64_t nps   = ms > 0 ? nodes * 1000ULL / ms : 0;

            bool isMate = std::abs(score) > 450'000;
            std::cout << "info multipv " << multipv
                      << " depth "       << d
                      << " score ";
            if (isMate) {
                int plies = 900'000 - std::abs(score);
                int moves = (plies + 1) / 2;
                int sign  = (score > 0) ? 1 : -1;
                std::cout << "mate " << sign * moves;
            } else {
                std::cout << "cp " << score;
            }
            std::cout << " nodes " << nodes
                      << " nps "   << nps
                      << " time "  << ms
                      << " pv";
            for (const Move& m : pv)
                std::cout << " " << UCI::moveToUCI(m);
            std::cout << "\n" << std::flush;
        };

        Move best = search_.findBestMove(board_, depth, cb);
        std::cout << "bestmove " << moveToUCI(best) << "\n" << std::flush;
    });
}

void UCI::handleStop() {
    search_.stop();
    if (searchThread_.joinable()) searchThread_.join();
}

// ── Static helpers ────────────────────────────────────────────────────────────

// "e2" → bit index using h1=bit0 board orientation.
int UCI::squareFromUCI(const std::string& s) {
    int file = s[0] - 'a'; // a=0 … h=7
    int rank = s[1] - '1'; // 1=0 … 8=7
    return rank * 8 + (7 - file);
}

std::string UCI::moveToUCI(const Move& m) {
    std::string s = Move::bitboardPositionToNotation(m.from)
                  + Move::bitboardPositionToNotation(m.to);
    if (m.moveType == Move::Promote || m.moveType == Move::PromoteCapture) {
        switch (m.newPiece) {
            case Move::Queen:  s += 'q'; break;
            case Move::Rook:   s += 'r'; break;
            case Move::Bishop: s += 'b'; break;
            case Move::Knight: s += 'n'; break;
            default: break;
        }
    }
    return s;
}

// Find the legal move that matches the UCI coordinate string.
Move UCI::parseMoveUCI(const std::string& uci) const {
    int from = squareFromUCI(uci.substr(0, 2));
    int to   = squareFromUCI(uci.substr(2, 2));

    bool             isPromo   = uci.length() >= 5;
    Move::PieceEnum  promoPiece = Move::Queen;
    if (isPromo) {
        switch (uci[4]) {
            case 'q': promoPiece = Move::Queen;  break;
            case 'r': promoPiece = Move::Rook;   break;
            case 'b': promoPiece = Move::Bishop; break;
            case 'n': promoPiece = Move::Knight; break;
            default:  break;
        }
    }

    MoveList legal = MoveGenerator::generateLegalMoves(board_);
    for (int i = 0; i < legal.count; i++) {
        const Move& m = legal.moves[i];
        if (m.from != from || m.to != to) continue;
        if (isPromo) {
            if ((m.moveType == Move::Promote || m.moveType == Move::PromoteCapture)
                && m.newPiece == promoPiece)
                return m;
        } else {
            return m;
        }
    }
    // Fallback — should not be reached with valid UCI input.
    return legal.moves[0];
}
