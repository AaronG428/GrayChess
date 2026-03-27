#include "Zobrist.h"
#include "../board/Board.h"

namespace Zobrist {

uint64_t PIECE_KEYS[14][64];
uint64_t SIDE_KEY;
uint64_t CASTLE_KEYS[16];
uint64_t EP_KEYS[8];

void init() {
    // TODO Phase 8: seed with std::mt19937_64, fill all key arrays
}

uint64_t compute(const Board& board) {
    // TODO Phase 8: XOR all occupied squares, side, castle rights, ep file
    (void)board;
    return 0;
}

} // namespace Zobrist
