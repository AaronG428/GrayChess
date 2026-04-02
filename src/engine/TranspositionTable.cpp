#include "TranspositionTable.h"
#include <cassert>



TranspositionTable::TranspositionTable(size_t numEntries)
    : entries_(numEntries), mask_(numEntries - 1) {
        assert(((numEntries & (numEntries-1)) == 0, "Check that numEntries is a power of 2"));
    }

TTEntry* TranspositionTable::probe(uint64_t hash) {
    size_t idx = hash & mask_;
    TTEntry* entry = &entries_[idx];
    if ((entry->type == NodeType::None) || (entry->hash != hash)){ //collision or uninitialized node
        return nullptr;
    }

    return entry;
}

void TranspositionTable::store(uint64_t hash, int score, int depth,
                               NodeType type, const Move& bestMove) {
    uint64_t idx = hash & mask_;
    TTEntry existing = entries_[idx];
    if (existing.type == NodeType::None || existing.depth <= depth){
        entries_[idx] = {hash, score, depth, type, bestMove};
    }         
}

void TranspositionTable::clear() {
    size_t numEntries = size(); 
    entries_.clear();
    entries_.resize(numEntries);
}
