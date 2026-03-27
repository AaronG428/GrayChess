#pragma once
#include <cstdint>

// Single-instruction bit utilities using compiler builtins (x86: BSF/TZCNT/POPCNT)

inline int lsb(uint64_t b)          { return __builtin_ctzll(b); }
inline int popcount(uint64_t b)     { return __builtin_popcountll(b); }

// Returns the least-significant set bit and clears it in b.
inline uint64_t popLSB(uint64_t& b) {
    uint64_t l = b & -b;
    b &= b - 1;
    return l;
}
