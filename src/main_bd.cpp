#include "bd.h"

int main() {
    bd banco = bd(0,1, new AtomicBroadcastRing(0, {{0, {"localhost", 3000}}, {1, {"localhost", 3001}}}, "config.txt", 0, 0));
    return 0;
}