#include "generator.h"

int main() {
    generator gen = generator(20, 50, 10000, new AtomicBroadcastRing(1, {{0, {"localhost", 3000}}, {1, {"localhost", 3001}}}, "config.txt", 0, 0));
    gen.run();
    return 0;
}