#pragma once

#include <span>
#include <cstdint>
#include <cassert>

#include "types.h"

namespace Chess {
class Bitset {
public:

    explicit Bitset(u64* data, size_t nb_bits)
        : nbits(nb_bits),
          w(data, nb_bits) {}

        

    size_t size() const { return nbits; }


    bool test(size_t i) const {
        assert(i < nbits);
        return (w[i>>6] >> (i & 63)) & 1ULL;
    }

    inline void set(size_t i)
    {
        assert(i < nbits);
        w[i >> 6] |= (1ULL << (i & 63));
    }

    inline void reset(size_t i) {
        assert(i < nbits);
        w[i >> 6] &= ~(1ULL << (i & 63));
    }

    void clear() {
        std::fill(w.begin(), w.end(), 0ULL);
    }

    friend Bitset& operator&=(Bitset& a, const Bitset& b);

    const char* data() {
        return reinterpret_cast<char*>(w.data());
    }

    private:
    size_t nbits = 0;
    std::span<u64> w;
};

Bitset &operator&=(Bitset &a, const Bitset &b);
}