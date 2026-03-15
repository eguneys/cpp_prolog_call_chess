#pragma once

#include <cassert>
#include <string>

#include "types.h"


namespace Chess {

    
    enum MoveType: u16 {
        NORMAL
    };

    // 16 bits
    // bit 0-5: destination square (0-63)
    // bit 6-11: origin square (0-63)
    // bit 12-13: promotion piece type - 2 (KNIGHT-2 to QUEEN-2)
    // bit 14-15: special move flat: promotion (1), en passant (2), castling (3)
    class Move
    {
    public:
        Move() = default;

        constexpr explicit Move(u16 d) : data(d) {}

        constexpr Move(Square from, Square to) : data((from << 6) + to) {}

        constexpr Square from_sq() const
        {
            assert(is_ok());
            return Square((data >> 6) & 0x3F);
        }

        constexpr Square to_sq() const
        {
            assert(is_ok());
            return Square(data & 0x3F);
        }



        constexpr MoveType type_of() const { return MoveType(data & (3 << 14)); }

        constexpr bool is_ok() const { return none().data != data && null().data != data; }

        static constexpr Move null() { return Move(65); }
        static constexpr Move none() { return Move(0); }

        static Move parse_uci(const std::string_view& uci);

        constexpr u16 raw() const { return data; }

        private:

        u16 data;
    };
}