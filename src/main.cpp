#include "move/AttackTables.h"
#include "engine/Zobrist.h"
#include "uci/UCI.h"

int main() {
    std::ios::sync_with_stdio(false);
    AttackTables::init();
    Zobrist::init();
    UCI uci;
    uci.loop();
    return 0;
}
