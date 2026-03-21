#include <iostream>
#include <tuple>
#include <sstream>

#include "duckdb.hpp"

#include "chess/types.h"
#include "chess/file_io.h"
#include "chess/gen_geometry.h"

void create_views(duckdb::Connection& con) {

}

void query_tables(duckdb::Connection& con) {
    std::cout << "Query Language Ready " <<std::endl;

    std::cout << "done." << std::endl;
}


void create_indexes(duckdb::Connection& con) {

    con.Query(""" \
        CREATE UNIQUE INDEX idx_turn ON turn(id); \
        CREATE INDEX idx_turn ON turn(Frm); \
        CREATE INDEX idx_turn ON turn(Role); \
        CREATE UNIQUE INDEX idx_opponent ON opponent(id); \
        CREATE INDEX idx_opponent ON opponent(Frm); \
        CREATE INDEX idx_opponent ON opponent(Role); \
        CREATE UNIQUE INDEX idx_turn_is_white ON turn_is_white(id); \
        CREATE INDEX idx_turn_is_white ON turn_is_white(is_white); \
        CREATE INDEX rook_line ON rook_line(frm); \
        CREATE INDEX rook_line ON rook_line(_to); \
        CREATE INDEX bishop_line ON bishop_line(frm); \
        CREATE INDEX bishop_line ON bishop_line(_to); \
        CREATE INDEX blocker_for ON blocker_for(frm); \
        CREATE INDEX blocker_for ON blocker_for(_to); \
        CREATE INDEX blocker_for ON blocker_for(block); \
        CREATE INDEX pawn_attack_geom ON pawn_attack_geom(frm); \
        CREATE INDEX pawn_attack_geom ON pawn_attack_geom(_to); \
        CREATE INDEX pawn_attack_geom ON pawn_attack_geom(is_white); \
        CREATE INDEX king_attack_geom ON king_attack_geom(frm); \
        CREATE INDEX king_attack_geom ON king_attack_geom(_to); \
        CREATE INDEX knight_attack_geom ON knight_attack_geom(frm); \
        CREATE INDEX knight_attack_geom ON knight_attack_geom(_to); \
        CREATE INDEX pawn_step ON pawn_step(frm); \
        CREATE INDEX pawn_step ON pawn_step(_to); \
        CREATE INDEX pawn_step ON pawn_step(is_white); \
\
        CREATE INDEX pawn_start_rank ON pawn_start_rank(start); \
        CREATE INDEX pawn_start_rank ON pawn_start_rank(is_white); \
\
        CREATE INDEX king_start_rank ON king_start_rank(start); \
        CREATE INDEX king_start_rank ON king_start_rank(is_white); \
\
        CREATE INDEX rook_start_rank ON rook_start_rank(start); \
        CREATE INDEX rook_start_rank ON rook_start_rank(is_white); \
        CREATE INDEX rook_start_rank ON rook_start_rank(is_king_side); \
\
        CREATE INDEX king_castle_to ON king_castle_to(_to); \
        CREATE INDEX king_castle_to ON king_castle_to(is_white); \
        CREATE INDEX king_castle_to ON king_castle_to(is_king_side); \
\
        CREATE INDEX rook_castle_to ON rook_castle_to(_to); \
        CREATE INDEX rook_castle_to ON rook_castle_to(is_white); \
        CREATE INDEX rook_castle_to ON rook_castle_to(is_rook_side); \
""");

}



void create_tables(duckdb::Connection& con) {

    /*
    con.Query("""CREATE TABLE IF NOT EXISTS position ( \
        id VARCHAR,        \
        white_occ UBIGINT, \
        pawn UBIGINT, \
        knight UBIGINT, \
        bishop UBIGINT, \
        rook UBIGINT, \
        queen UBIGINT, \
        king UBIGINT, \
        best_move UBIGINT, \
        turn BOOLEAN)""");
        */


    // std::string csv_file = "../data/athousand_sorted.csv";
    std::string csv_file = "../data/lichess_db_puzzle.csv";
    std::string db_file = "../data/db_out.chess_db";
    Chess::build_csv_parse_and_records(csv_file, db_file, true);

    Chess::DBReader reader(db_file.c_str());

    u64 puzzles_in_check;
    u64 total_puzzles = reader.size();

    auto result = con.Query("""CREATE TABLE IF NOT EXISTS turn ( \
        id VARCHAR,        \
        Frm UINT8, \
        Role UINT8)""");

    if (result->HasError()) {
        std::cerr << "Error: " << result->GetError() << std::endl;
    }

    con.Query("""CREATE TABLE IF NOT EXISTS opponent ( \
        id VARCHAR,        \
        Frm UINT8, \
        Role UINT8)""");


    duckdb::Appender turn(con, "turn");

    u8 index_in_puzzle_id = 0;
    u64 last_puzzle_id = 0;
    puzzles_in_check = 0;
    reader.sweep([&](const Chess::PositionRecord &rec, const Chess::PositionFeatures &feat)
                 {


                     puzzles_in_check++;
                     int Progress = (static_cast<float>(puzzles_in_check) / total_puzzles) * 100;
                     if (Progress % 10 == 0) {
                         std::cout << "\r 1/2 Progress: %" << Progress << " ";
                         std::cout << std::flush;
                     }

                    if (puzzles_in_check > 10) {

                        //return;
                    }

                    if (last_puzzle_id != feat.id) {
                        index_in_puzzle_id = 0;
                        last_puzzle_id = feat.id;
                    } else {
                        index_in_puzzle_id++;
                    }

                     duckdb::Value id = duckdb::Value(Chess::decode_ascii_id(feat.id) + std::string("_") + std::string(1, '0' + index_in_puzzle_id));

                     for (int i = 0; i < 6; i++)
                     {

                         Chess::PieceType role = Chess::PieceType(i + 1);
                         Chess::Bitboard turn_bb = rec.pieces[i] & rec.white_occ;
                         Chess::Bitboard opp_bb = rec.pieces[i] ^ turn_bb;

                         if (rec.state == Chess::Black)
                         {
                             std::swap(turn_bb, opp_bb);
                         }

                         while (turn_bb)
                         {
                             Chess::Square from = Chess::pop_lsb(turn_bb);
                             turn.AppendRow(id, duckdb::Value(from), duckdb::Value(role));
                         }
                     }

                 });

    turn.Close();

    duckdb::Appender opponent(con, "opponent");

    puzzles_in_check = 0;
    reader.sweep([&](const Chess::PositionRecord &rec, const Chess::PositionFeatures &feat)
                 {


                     puzzles_in_check++;
                     int Progress = (static_cast<float>(puzzles_in_check) / total_puzzles) * 100;
                     if (Progress % 10 == 0)
                     {
                         std::cout << "\r 2/2 Progress: %" << Progress << " ";
                         std::cout << std::flush;
                     }

                    if (puzzles_in_check > 10) {

                        //return;
                    }

                    if (last_puzzle_id != feat.id)
                    {
                        index_in_puzzle_id = 0;
                        last_puzzle_id = feat.id;
                    } else {
                        index_in_puzzle_id++;
                    }

                     duckdb::Value id = duckdb::Value(Chess::decode_ascii_id(feat.id) + std::string("_") + std::string(1, '0' + index_in_puzzle_id));

                     for (int i = 0; i < 6; i++)
                     {
                         Chess::PieceType role = Chess::PieceType(i + 1);
                         Chess::Bitboard turn_bb = rec.pieces[i] & rec.white_occ;
                         Chess::Bitboard opp_bb = rec.pieces[i] ^ turn_bb;

                         if (rec.state == Chess::Black)
                         {
                             std::swap(turn_bb, opp_bb);
                         }

                         while (opp_bb)
                         {
                             Chess::Square from = Chess::pop_lsb(opp_bb);
                             opponent.AppendRow(id, duckdb::Value(from), duckdb::Value(role));
                         }
                     }

                 });

    opponent.Close();

        

    std::cout << "Done 1/4." <<std::endl;
}

void create_tables2(duckdb::Connection& con) {
    std::string db_file = "../data/db_out.chess_db";
    Chess::DBReader reader(db_file.c_str());

    auto result = con.Query("""CREATE TABLE IF NOT EXISTS turn_is_white ( \
        id VARCHAR,        \
        is_white BOOL)""");

    if (result->HasError()) {
        std::cerr << "Error: " << result->GetError() << std::endl;
    }


    duckdb::Appender turn_is_white(con, "turn_is_white");

    u64 last_puzzle_id = 0;
    u8 index_in_puzzle_id = 0;
    reader.sweep([&](const Chess::PositionRecord &rec, const Chess::PositionFeatures &feat)
                 {
                     if (last_puzzle_id != feat.id)
                     {
                         index_in_puzzle_id = 0;
                         last_puzzle_id = feat.id;
                     }
                     else
                     {
                         index_in_puzzle_id++;
                     }

                     duckdb::Value id = duckdb::Value(Chess::decode_ascii_id(feat.id) + std::string("_") + std::string(1, '0' + index_in_puzzle_id));

                     bool is_white = rec.state == Chess::White;

                     turn_is_white.AppendRow(id, duckdb::Value(is_white));
                 });


                 turn_is_white.Close();



 
    result = con.Query(""" \
        CREATE TABLE IF NOT EXISTS vacant_see ( \
            id VARCHAR,        \
            frm UINT8, \
            _to UINT8); \
       \
 CREATE TABLE IF NOT EXISTS defend_see ( \
            id VARCHAR,        \
            frm UINT8, \
            _to UINT8); \
       \
 CREATE TABLE IF NOT EXISTS attack_see ( \
            id VARCHAR,        \
            frm UINT8, \
            _to UINT8); \
\
        CREATE TABLE IF NOT EXISTS vacant_see2 ( \
            id VARCHAR,        \
            frm UINT8, \
            _to UINT8,  \
            _to2 UINT8 \
            ); \
       \
 CREATE TABLE IF NOT EXISTS defend_see2 ( \
            id VARCHAR,        \
            frm UINT8, \
            _to UINT8 \
            _to2 UINT8 \
            ); \
       \
 CREATE TABLE IF NOT EXISTS attack_see2 ( \
            id VARCHAR,        \
            frm UINT8, \
            _to UINT8 \
            _to2 UINT8 \
            ); \
        """);

    if (result->HasError()) {
        std::cerr << "Error: " << result->GetError() << std::endl;
    }

    duckdb::Appender vacant_see(con, "vacant_see");
    duckdb::Appender defend_see(con, "defend_see");
    duckdb::Appender attack_see(con, "attack_see");
    duckdb::Appender vacant_see2(con, "vacant_see2");
    duckdb::Appender defend_see2(con, "defend_see2");
    duckdb::Appender attack_see2(con, "attack_see2");

    u64 total_puzzles = reader.size();
    u64 puzzles_in_check = 0;
    last_puzzle_id = 0;
    index_in_puzzle_id = 0;
    reader.sweep([&](const Chess::PositionRecord &rec, const Chess::PositionFeatures &feat)
                 {
                     puzzles_in_check++;
                     int Progress = (static_cast<float>(puzzles_in_check) / total_puzzles) * 100;
                     if (Progress % 10 == 0)
                     {
                         std::cout << "\r Progress: %" << Progress << " ";
                         std::cout << std::flush;
                     }



                     if (last_puzzle_id != feat.id)
                     {
                         index_in_puzzle_id = 0;
                         last_puzzle_id = feat.id;
                     }
                     else
                     {
                         index_in_puzzle_id++;
                     }

                     duckdb::Value id = duckdb::Value(Chess::decode_ascii_id(feat.id) + std::string("_") + std::string(1, '0' + index_in_puzzle_id));

                     Chess::Bitboard occupied =
                         rec.pieces[0] |
                         rec.pieces[1] |
                         rec.pieces[2] |
                         rec.pieces[3] |
                         rec.pieces[4] |
                         rec.pieces[5];

                     Chess::Bitboard from_bb = occupied;

                     while (from_bb)
                     {

                         Chess::Square from = Chess::pop_lsb(from_bb);
                         Chess::Piece piece = Chess::piece_on(rec, from);

                         Chess::Bitboard aa = Chess::attacks_bb(Chess::typeof_piece(piece), from, occupied);

                         while (aa)
                         {
                             Chess::Square to = Chess::pop_lsb(aa);

                             Chess::Piece piece2 = Chess::piece_on(rec, to);

                             bool is_vacant = piece2 == Chess::No_Piece;

                             bool is_defend = (rec.white_occ & from) == (rec.white_occ & to);

                             if (is_vacant)
                             {
                                 vacant_see.AppendRow(id, duckdb::Value(from), duckdb::Value(to));
                             }
                             else if (is_defend)
                             {
                                 defend_see.AppendRow(id, duckdb::Value(from), duckdb::Value(to));
                             }
                             else
                             {
                                 attack_see.AppendRow(id, duckdb::Value(from), duckdb::Value(to));
                             }

                             Chess::Bitboard aa2 = Chess::attacks_bb(Chess::typeof_piece(piece), to, occupied ^ from);

                             while (aa2)
                             {
                                 Chess::Square to2 = Chess::pop_lsb(aa2);

                                 Chess::Piece piece3 = Chess::piece_on(rec, to2);

                                 bool is_vacant = piece3 == Chess::No_Piece;

                                 bool is_defend = (rec.white_occ & from) == (rec.white_occ & to2);

                                 if (is_vacant)
                                 {
                                     vacant_see2.AppendRow(id, duckdb::Value(from), duckdb::Value(to), duckdb::Value(to2));
                                 }
                                 else if (is_defend)
                                 {
                                     defend_see2.AppendRow(id, duckdb::Value(from), duckdb::Value(to), duckdb::Value(to2));
                                 }
                                 else
                                 {
                                     attack_see2.AppendRow(id, duckdb::Value(from), duckdb::Value(to), duckdb::Value(to2));
                                 }
                             }
                         }
                     }
                 });

    turn_is_white.Close();


    vacant_see.Close();
    attack_see.Close();
    defend_see.Close();


       


    std::cout << "Done 2/4." <<std::endl;
}

void create_tables_3(duckdb::Connection& con) {

    auto result = con.Query("""CREATE TABLE IF NOT EXISTS rook_line ( \
        frm UINT8,        \
        _to UINT8)""");

    if (result->HasError()) {
        std::cerr << "Error: " << result->GetError() << std::endl;
    }



    con.Query("""CREATE TABLE IF NOT EXISTS bishop_line ( \
        frm UINT8,        \
        _to UINT8)""");

    con.Query("""CREATE TABLE IF NOT EXISTS blocker_for ( \
        frm UINT8,        \
        _to UINT8, \
        block UINT8)""");

    con.Query("""CREATE TABLE IF NOT EXISTS pawn_attack_geom ( \
        frm UINT8,        \
        _to UINT8, \
        is_white BOOL)""");

    con.Query("""CREATE TABLE IF NOT EXISTS king_attack_geom ( \
        frm UINT8,        \
        _to UINT8)""");

    con.Query("""CREATE TABLE IF NOT EXISTS knight_attack_geom ( \
        frm UINT8,        \
        _to UINT8)""");

    con.Query("""CREATE TABLE IF NOT EXISTS pawn_step ( \
        frm UINT8,        \
        _to UINT8, \
        is_white BOOL)""");


    duckdb::Appender rook_line(con, "rook_line");
    duckdb::Appender bishop_line(con, "bishop_line");
    duckdb::Appender blocker_for(con, "blocker_for");
    duckdb::Appender pawn_attack_geom(con, "pawn_attack_geom");
    duckdb::Appender king_attack_geom(con, "king_attack_geom");
    duckdb::Appender knight_attack_geom(con, "knight_attack_geom");
    duckdb::Appender pawn_step(con, "pawn_step");


    std::vector<std::tuple<Chess::Color, Chess::Square, Chess::Square>> v_pawn_step = Chess::gen_pawn_step();
    std::vector<std::tuple<Chess::Color, Chess::Square, Chess::Square>> v_pawn_attack_geom = Chess::gen_pawn_attack_geom();
    std::vector<std::tuple<Chess::Square, Chess::Square>> v_king_attack_geom = Chess::gen_king_attack_geom();
    std::vector<std::tuple<Chess::Square, Chess::Square>> v_knight_attack_geom = Chess::gen_knight_attack_geom();


    std::vector<std::tuple<Chess::Square, Chess::Square>> v_rook_lines = Chess::gen_rook_lines();
    std::vector<std::tuple<Chess::Square, Chess::Square>> v_bishop_lines = Chess::gen_bishop_lines();
    std::vector<std::tuple<Chess::Square, Chess::Square, Chess::Square>> v_blockers_for = Chess::gen_blockers_for();


    for (const auto& [color, from, to] : v_pawn_step) {
        pawn_step.AppendRow(duckdb::Value(from), duckdb::Value(to), duckdb::Value(color==Chess::White));
    }
    for (const auto& [color, from, to] : v_pawn_attack_geom) {
        pawn_attack_geom.AppendRow(duckdb::Value(from), duckdb::Value(to), duckdb::Value(color==Chess::White));
    }
    for (const auto& [from, to] : v_king_attack_geom) {
        king_attack_geom.AppendRow(duckdb::Value(from), duckdb::Value(to));
    }
    for (const auto& [from, to] : v_knight_attack_geom) {
        knight_attack_geom.AppendRow(duckdb::Value(from), duckdb::Value(to));
    }

    for (const auto& [from, to] : v_rook_lines) {
        rook_line.AppendRow(duckdb::Value(from), duckdb::Value(to));
    }

    for (const auto& [from, to] : v_bishop_lines) {
        bishop_line.AppendRow(duckdb::Value(from), duckdb::Value(to));
    }
    for (const auto& [from, to, block] : v_blockers_for) {
        blocker_for.AppendRow(duckdb::Value(from), duckdb::Value(to), duckdb::Value(block));
    }

    rook_line.Close();
    bishop_line.Close();
    blocker_for.Close();
    pawn_attack_geom.Close();
    king_attack_geom.Close();
    knight_attack_geom.Close();
    pawn_step.Close();

    std::cout << "Done 3/4." <<std::endl;
}

void create_tables_4(duckdb::Connection& con) {

    con.Query("""CREATE TABLE IF NOT EXISTS pawn_start_rank ( \
        start UINT8,        \
        is_white BOOL)""");

    con.Query("""CREATE TABLE IF NOT EXISTS king_start ( \
        start UINT8,        \
        is_white BOOL)""");

    con.Query("""CREATE TABLE IF NOT EXISTS rook_start ( \
        start UINT8,        \
        is_king_side BOOL,        \
        is_white BOOL)""");

    con.Query("""CREATE TABLE IF NOT EXISTS king_castle_to ( \
        _to UINT8,        \
        is_king_side BOOL,        \
        is_white BOOL)""");

    con.Query("""CREATE TABLE IF NOT EXISTS rook_castle_to ( \
        _to UINT8,        \
        is_king_side BOOL,        \
        is_white BOOL)""");

    std::stringstream pawn_start_rank;

    #define INSERT(start, is_white) \
    pawn_start_rank << "INSERT INTO pawn_start_rank VALUES("; \
    pawn_start_rank << start; \
    pawn_start_rank << ","; \
    pawn_start_rank << is_white; \
    pawn_start_rank << ");";

    INSERT(Chess::A2, true)
    INSERT(Chess::B2, true)
    INSERT(Chess::C2, true)
    INSERT(Chess::D2, true)
    INSERT(Chess::E2, true)
    INSERT(Chess::F2, true)
    INSERT(Chess::G2, true)
    INSERT(Chess::H2, true)

    INSERT(Chess::A7, false)
    INSERT(Chess::B7, false)
    INSERT(Chess::C7, false)
    INSERT(Chess::D7, false)
    INSERT(Chess::E7, false)
    INSERT(Chess::F7, false)
    INSERT(Chess::G7, false)
    INSERT(Chess::H7, false)

   #undef INSERT


    std::stringstream king_start;

    king_start << "INSERT INTO king_start VALUES("; \
    king_start << Chess::E1; \
    king_start << ","; \
    king_start << true; \
    king_start << ");";

    king_start << "INSERT INTO king_start VALUES("; \
    king_start << Chess::E8; \
    king_start << ","; \
    king_start << false; \
    king_start << ");";



    std::stringstream rook_start;

#define INSERT(start, is_king_side, is_white)       \
    rook_start << "INSERT INTO rook_start VALUES("; \
    rook_start << start;                            \
    rook_start << ",";                              \
    rook_start << is_king_side;                     \
    rook_start << ",";                              \
    rook_start << is_white;                         \
    rook_start << ");";

    INSERT(Chess::H1, true, true)
    INSERT(Chess::A1, false, true)
    INSERT(Chess::H8, true, false)
    INSERT(Chess::A8, false, false)

#undef INSERT


    std::stringstream king_castle_to;

#define INSERT(start, is_king_side, is_white)       \
    king_castle_to << "INSERT INTO king_castle_to VALUES("; \
    king_castle_to << start;                            \
    king_castle_to << ",";                              \
    king_castle_to << is_king_side;                     \
    king_castle_to << ",";                              \
    king_castle_to << is_white;                         \
    king_castle_to << ");";

    INSERT(Chess::G1, true, true)
    INSERT(Chess::C1, false, true)
    INSERT(Chess::G8, true, false)
    INSERT(Chess::C8, false, false)

#undef INSERT



    std::stringstream rook_castle_to;

#define INSERT(start, is_king_side, is_white)       \
    rook_castle_to << "INSERT INTO rook_castle_to VALUES("; \
    rook_castle_to << start;                            \
    rook_castle_to << ",";                              \
    rook_castle_to << is_king_side;                     \
    rook_castle_to << ",";                              \
    rook_castle_to << is_white;                         \
    rook_castle_to << ");";

    INSERT(Chess::F1, true, true)
    INSERT(Chess::D1, false, true)
    INSERT(Chess::F8, true, false)
    INSERT(Chess::D8, false, false)

#undef INSERT



    con.Query(pawn_start_rank.str());
    con.Query(king_start.str());
    con.Query(rook_start.str());
    con.Query(king_castle_to.str());
    con.Query(rook_castle_to.str());
}


void init_db(duckdb::Connection& con) {

    try {
    create_tables(con);
    create_tables2(con);
    create_tables_3(con);
    create_tables_4(con);
    //create_indexes(con);

    create_views(con);
    query_tables(con);
    } catch (const std::runtime_error& e) {
        con.~Connection();
    }


}

int main() {

    Chess::Bitboards::init();

    duckdb::DuckDB db("../data/duckdb.chess.db", nullptr);
    duckdb::Connection con(db);
    init_db(con);
    return 0;
}