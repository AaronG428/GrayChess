#include "TranspositionTable.h"

TranspositionTable::TranspositionTable(size_t numEntries)
    : entries_(numEntries), mask_(numEntries - 1) {}

TTEntry* TranspositionTable::probe(uint64_t hash) {
    // TODO Phase 8: return &entries_[hash & mask_] if entry->hash == hash, else nullptr
    (void)hash;
    return nullptr;
}

void TranspositionTable::store(uint64_t hash, int score, int depth,
                               NodeType type, const Move& bestMove) {
    // TODO Phase 8: entries_[hash & mask_] = {hash, score, depth, type, bestMove}
    //               if depth >= existing entry's depth
    (void)hash; (void)score; (void)depth; (void)type; (void)bestMove;
}

void TranspositionTable::clear() {
    // TODO Phase 8: fill entries_ with zeroed TTEntry{}
}
