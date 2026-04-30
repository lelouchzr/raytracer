#include "rt/Extensions.hpp"

namespace rt {
double random01(unsigned long long &state) {
    state ^= state >> 12;
    state ^= state << 25;
    state ^= state >> 27;
    return double((state * 2685821657736338717ULL) >> 11) * (1.0 / 9007199254740992.0);
}

} // namespace rt
