#include "gen_geometry.h"
#include "types.h"
#include "bitboard.h"

namespace Chess {


    void Push_if_in_bounds(std::vector<std::tuple<Chess::Square, Chess::Square>>& res, File ai, Rank aj, int i, int j) {

        if (i >= File_A && i <= File_H && j >= Rank_1 && j <= Rank_8) {
            res.emplace_back(make_square(ai, aj), make_square(File(i), Rank(j)));
        }

    }

    std::vector<std::tuple<Chess::Color, Chess::Square, Chess::Square>> gen_pawn_step() {

        std::vector<std::tuple<Chess::Color, Chess::Square, Chess::Square>> res = {};

        for (File i = File_A; i <= File_H; ++i) {
            for (Rank j = Rank_2; j < Rank_8; ++j) {
                res.emplace_back(White, make_square(i, j), make_square(i, Rank(j + 1)));
                res.emplace_back(Black, make_square(i, j), make_square(i, Rank(j - 1)));
            }
        }

        return res;
    }

    std::vector<std::tuple<Chess::Color, Chess::Square, Chess::Square>> gen_pawn_attack_geom() {

        std::vector<std::tuple<Chess::Color, Chess::Square, Chess::Square>> res = {};

        for (File i = File_A; i <= File_H; ++i) {
            for (Rank j = Rank_2; j < Rank_8; ++j) {
                if (i < File_H) res.emplace_back(White, make_square(i, j), make_square(File(i + 1), Rank(j + 1)));
                if (i > File_A) res.emplace_back(White, make_square(i, j), make_square(File(i - 1), Rank(j + 1)));
                if (i < File_H) res.emplace_back(Black, make_square(i, j), make_square(File(i + 1), Rank(j - 1)));
                if (i > File_A) res.emplace_back(Black, make_square(i, j), make_square(File(i - 1), Rank(j - 1)));
            }
        }

        return res;
    }

    std::vector<std::tuple<Chess::Square, Chess::Square>> gen_knight_attack_geom() {

        std::vector<std::tuple<Chess::Square, Chess::Square>> res = {};

        for (File i = File_A; i <= File_H; ++i) {
            for (Rank j = Rank_1; j <= Rank_8; ++j)
            {
                Push_if_in_bounds(res, i, j, i + 2, j + 1);
                Push_if_in_bounds(res, i, j, i - 2, j + 1);
                Push_if_in_bounds(res, i, j, i + 1, j + 2);
                Push_if_in_bounds(res, i, j, i + 1, j - 2);
                Push_if_in_bounds(res, i, j, i - 1, j + 2);
                Push_if_in_bounds(res, i, j, i - 1, j - 2);
                Push_if_in_bounds(res, i, j, i + 2, j - 1);
                Push_if_in_bounds(res, i, j, i - 2, j - 1);
            }
        }
        return res;
    }


    std::vector<std::tuple<Chess::Square, Chess::Square>> gen_king_attack_geom() {
        std::vector<std::tuple<Chess::Square, Chess::Square>> res = {};

        int step = 1;
        for (File i = File_A; i <= File_H; ++i) {
            for (Rank j = Rank_1; j <= Rank_8; ++j)
            {
                Push_if_in_bounds(res, i, j, i + step, j);
                Push_if_in_bounds(res, i, j, i - step, j);
                Push_if_in_bounds(res, i, j, i, j + step);
                Push_if_in_bounds(res, i, j, i, j - step);
                Push_if_in_bounds(res, i, j, i + step, j + step);
                Push_if_in_bounds(res, i, j, i + step, j - step);
                Push_if_in_bounds(res, i, j, i - step, j + step);
                Push_if_in_bounds(res, i, j, i - step, j - step);
            }
        }
        return res;
    }


    std::vector<std::tuple<Chess::Square, Chess::Square>> gen_rook_lines() {
        std::vector<std::tuple<Chess::Square, Chess::Square>> res = {};

        for (File i = File_A; i <= File_H; ++i) {
            for (Rank j = Rank_1; j <= Rank_8; ++j)
            {
                for (int step = 1; step < 8; step++) {
                    Push_if_in_bounds(res, i, j, i + step, j);
                    Push_if_in_bounds(res, i, j, i - step, j);
                    Push_if_in_bounds(res, i, j, i, j + step);
                    Push_if_in_bounds(res, i, j, i, j - step);
                }
            }
        }
        return res;
    }

    std::vector<std::tuple<Chess::Square, Chess::Square>> gen_bishop_lines() {
        std::vector<std::tuple<Chess::Square, Chess::Square>> res = {};

        for (File i = File_A; i <= File_H; ++i) {
            for (Rank j = Rank_1; j <= Rank_8; ++j)
            {
                for (int step = 1; step < 8; step++)
                {
                    Push_if_in_bounds(res, i, j, i + step, j + step);
                    Push_if_in_bounds(res, i, j, i + step, j - step);
                    Push_if_in_bounds(res, i, j, i - step, j + step);
                    Push_if_in_bounds(res, i, j, i - step, j - step);
                }
            }
        }
        return res;
    }

    bool check_is_mid_line (File i, Rank j, File i2, Rank j2, File mi, Rank mj)
    {
        return between_bb(make_square(i, j), make_square(i2, j2)) & square_bb(make_square(mi, mj));
    }

    void Push_if_in_mid(std::vector<std::tuple<Chess::Square, Chess::Square, Chess::Square>>& res, File i, Rank j, File i2, Rank j2, File mi, Rank mj) {

        bool is_in_mid = check_is_mid_line(i, j, i2, j2, mi, mj);
        if (is_in_mid) {
            res.emplace_back(make_square(i, j), make_square(i2, j2), make_square(mi, mj));
        }

    }

    std::vector<std::tuple<Chess::Square, Chess::Square, Chess::Square>> gen_blockers_for()
    {
        std::vector<std::tuple<Chess::Square, Chess::Square, Chess::Square>> res = {};

        for (File i = File_A; i <= File_H; ++i)
        {
            for (Rank j = Rank_1; j <= Rank_8; ++j)
            {

                for (File i2 = File_A; i2 <= File_H; ++i2)
                {
                    for (Rank j2 = Rank_1; j2 <= Rank_8; ++j2)
                    {

                        for (File mi = File_A; mi <= File_H; ++mi)
                        {
                            for (Rank mj = Rank_1; mj <= Rank_8; ++mj)
                            {
                                Push_if_in_mid(res, i, j, i2, j2, mi, mj);
                            }
                        }
                    }
                }
            }
        }
        return res;
    }
}