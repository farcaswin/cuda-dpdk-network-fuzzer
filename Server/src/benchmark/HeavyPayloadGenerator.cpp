#include "HeavyPayloadGenerator.h"
#include <cstring>
#include <cstdint>

namespace fuzzer {

// Same algorithm as GPU for fairness
static inline uint32_t xorshift32(uint32_t* state) {
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static inline uint32_t compute_crc32(const uint8_t* data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
    }
    return ~crc;
}

void HeavyPayloadGenerator::generate(uint8_t* output, uint16_t* lengths, uint32_t batch_size, const PacketGenConfig& config) {
    for (uint32_t idx = 0; idx < batch_size; idx++) {
        uint8_t* pkt = output + (static_cast<size_t>(idx) * 1514);
        
        // Headers (Simplified for benchmark speed, but same byte count)
        std::memset(pkt, 0, 14 + 20 + 8);
        
        // Heavy Computation
        uint8_t* payload = pkt + 14 + 20 + 8;
        uint32_t prng_state = idx + 1;
        
        uint32_t* payload32 = (uint32_t*)payload;
        for (int i = 0; i < 256; i++) {
            payload32[i] = xorshift32(&prng_state);
        }

        uint32_t crc = compute_crc32(payload, 1024);
        *((uint32_t*)(payload + 1020)) = crc;

        lengths[idx] = 14 + 20 + 8 + 1024;
    }
}

} // namespace fuzzer
