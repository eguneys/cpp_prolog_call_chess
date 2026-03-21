#pragma once

#include "types.h"
#include <vector>

namespace Chess {
    std::vector<std::tuple<Chess::Color, Chess::Square, Chess::Square>> gen_pawn_step();
    std::vector<std::tuple<Chess::Color, Chess::Square, Chess::Square>> gen_pawn_attack_geom();
    std::vector<std::tuple<Chess::Square, Chess::Square>> gen_king_attack_geom();
    std::vector<std::tuple<Chess::Square, Chess::Square>> gen_knight_attack_geom();


    std::vector<std::tuple<Chess::Square, Chess::Square>> gen_rook_lines();
    std::vector<std::tuple<Chess::Square, Chess::Square>> gen_bishop_lines();
    std::vector<std::tuple<Chess::Square, Chess::Square, Chess::Square>> gen_blockers_for();
}