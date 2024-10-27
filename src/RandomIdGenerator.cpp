#include "RandomIdGenerator.hpp"
#include <cstdint>
#include <random>

namespace IDGenerator {

uint64_t newID() {
    static std::random_device rd;     // Seed source
    static std::mt19937_64 rng(rd()); // 64-bit Mersenne Twister
    std::uniform_int_distribution<uint64_t> dist(
        0, UINT64_MAX); // Full range of uint64_t
    return dist(rng);   // Generate random 64-bit integer
}

}