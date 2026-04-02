#pragma once
#include <cstdint>
#include <vector>
#include "../move/Move.h"

// Node type — encodes what the stored score means relative to the
// alpha-beta window that was in effect when the position was searched.
enum class NodeType : uint8_t {
    None        = 0,
    Exact       = 1, // score is the true minimax value
    LowerBound  = 2, // score caused a beta cutoff; true value >= score
    UpperBound  = 3, // score was below alpha; true value <= score
};

// Single transposition table entry.
struct TTEntry {
    uint64_t hash     = 0;
    int      score    = 0;
    int      depth    = 0;       // remaining depth when this entry was stored
    NodeType type     = NodeType::None;
    Move     bestMove = {};      // best / refutation move found at this node
};

// Fixed-size power-of-two hash table.
// Index = hash & (size - 1); replace-if-deeper replacement policy.
class TranspositionTable {
public:
    // Construct with the given number of entries (must be a power of two).
    // Default: 1 << 22  ≈ 4M entries (~150 MB).
    explicit TranspositionTable(size_t numEntries = 1u << 22);

    // Look up a position by its Zobrist hash.
    // Returns nullptr if the entry is absent or belongs to a different position.
    TTEntry* probe(uint64_t hash);

    // Store a result for a position.
    // Replaces the existing entry if depth >= stored depth.
    void store(uint64_t hash, int score, int depth, NodeType type, const Move& bestMove);

    // Wipe all entries (call on ucinewgame).
    void clear();

    // Return the number of entries the table was sized for.
    size_t size() const { return entries_.size(); }

private:
    std::vector<TTEntry> entries_;
    size_t               mask_;   // entries_.size() - 1
};
