/*
 * SHA-256 implementation by Brad Conte (brad@bradconte.com)
 * Public domain. No warranty implied; use at your own risk.
 * Adapted from: https://github.com/B-Con/crypto-algorithms
 *
 * Only HashUtils::sha256(), generateSalt(), hashWithSalt(), and verify()
 * are part of the NovaBanc public API. The SHA-256 internals below are
 * intentionally kept as a self-contained, readable reference implementation.
 */

#include "HashUtils.h"

#include <cstdint>
#include <cstring>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>

// ─── SHA-256 internals ────────────────────────────────────────────────────────

namespace {

// Initial hash values (first 32 bits of the fractional parts of the square
// roots of the first 8 primes).
static const uint32_t H0[8] = {
    0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
    0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u
};

// Round constants (first 32 bits of the fractional parts of the cube roots
// of the first 64 primes).
static const uint32_t K[64] = {
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u,
    0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u,
    0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu,
    0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u,
    0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u,
    0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u,
    0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u,
    0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
    0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u
};

inline uint32_t rotr32(uint32_t x, uint32_t n) {
    return (x >> n) | (x << (32u - n));
}

// Process one 512-bit block.
void sha256_transform(uint32_t state[8], const uint8_t block[64]) {
    uint32_t w[64];
    uint32_t a, b, c, d, e, f, g, h;

    for (int i = 0; i < 16; ++i) {
        w[i] = (static_cast<uint32_t>(block[i * 4])     << 24) |
               (static_cast<uint32_t>(block[i * 4 + 1]) << 16) |
               (static_cast<uint32_t>(block[i * 4 + 2]) <<  8) |
               (static_cast<uint32_t>(block[i * 4 + 3]));
    }
    for (int i = 16; i < 64; ++i) {
        uint32_t s0 = rotr32(w[i-15], 7) ^ rotr32(w[i-15], 18) ^ (w[i-15] >> 3);
        uint32_t s1 = rotr32(w[i-2], 17) ^ rotr32(w[i-2], 19)  ^ (w[i-2] >> 10);
        w[i] = w[i-16] + s0 + w[i-7] + s1;
    }

    a = state[0]; b = state[1]; c = state[2]; d = state[3];
    e = state[4]; f = state[5]; g = state[6]; h = state[7];

    for (int i = 0; i < 64; ++i) {
        uint32_t S1    = rotr32(e, 6) ^ rotr32(e, 11) ^ rotr32(e, 25);
        uint32_t ch    = (e & f) ^ (~e & g);
        uint32_t temp1 = h + S1 + ch + K[i] + w[i];
        uint32_t S0    = rotr32(a, 2) ^ rotr32(a, 13) ^ rotr32(a, 22);
        uint32_t maj   = (a & b) ^ (a & c) ^ (b & c);
        uint32_t temp2 = S0 + maj;

        h = g; g = f; f = e; e = d + temp1;
        d = c; c = b; b = a; a = temp1 + temp2;
    }

    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

// Compute SHA-256 of arbitrary bytes; result written to digest[32].
void sha256_raw(const uint8_t* data, size_t len, uint8_t digest[32]) {
    uint32_t state[8];
    for (int i = 0; i < 8; ++i) state[i] = H0[i];

    uint8_t block[64];
    size_t pos = 0;

    // Process complete 512-bit blocks.
    while (pos + 64 <= len) {
        sha256_transform(state, data + pos);
        pos += 64;
    }

    // Remaining bytes + padding.
    size_t remaining = len - pos;
    std::memcpy(block, data + pos, remaining);
    block[remaining] = 0x80;
    std::memset(block + remaining + 1, 0, 64 - remaining - 1);

    if (remaining >= 56) {
        // Need an extra block for the length field.
        sha256_transform(state, block);
        std::memset(block, 0, 56);
    }

    // Append bit-length as big-endian uint64.
    uint64_t bitLen = static_cast<uint64_t>(len) * 8;
    for (int i = 7; i >= 0; --i) {
        block[56 + i] = static_cast<uint8_t>(bitLen & 0xFF);
        bitLen >>= 8;
    }
    sha256_transform(state, block);

    // Write digest in big-endian order.
    for (int i = 0; i < 8; ++i) {
        digest[i * 4]     = (state[i] >> 24) & 0xFF;
        digest[i * 4 + 1] = (state[i] >> 16) & 0xFF;
        digest[i * 4 + 2] = (state[i] >>  8) & 0xFF;
        digest[i * 4 + 3] =  state[i]        & 0xFF;
    }
}

} // anonymous namespace

// ─── HashUtils implementation ─────────────────────────────────────────────────

namespace novabanc {

std::string HashUtils::sha256(const std::string& input) {
    uint8_t digest[32];
    sha256_raw(reinterpret_cast<const uint8_t*>(input.data()), input.size(), digest);

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (uint8_t byte : digest) {
        oss << std::setw(2) << static_cast<unsigned>(byte);
    }
    return oss.str();
}

std::string HashUtils::generateSalt() {
    // 16 random bytes encoded as a 32-char lowercase hex string.
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<unsigned> dist(0, 255);

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (int i = 0; i < 16; ++i) {
        oss << std::setw(2) << dist(rng);
    }
    return oss.str();
}

std::string HashUtils::hashWithSalt(const std::string& input, const std::string& salt) {
    return sha256(input + salt);
}

bool HashUtils::verify(const std::string& input,
                       const std::string& salt,
                       const std::string& storedHash) {
    return hashWithSalt(input, salt) == storedHash;
}

} // namespace novabanc
