#pragma once

#include <iostream>

#include "file_io.h"
#include "bitboard.h"

namespace Chess {

    void uci_loop()
    {
        Chess::Bitboards::init();

        //std::string csv_file = "../data/athousand_sorted.csv";
        std::string csv_file = "../data/lichess_db_puzzle.csv";
        std::string db_file = "../data/db_out.chess_db";
        Chess::build_csv_parse_and_records(csv_file, db_file, true);

        Chess::DBReader reader(db_file.c_str());

        u64 puzzles_in_check = 0;

        // The lambda is the "Function Pointer" equivalent
        reader.sweep([&](const PositionRecord &rec, const PositionFeatures &feat)
                     {
        // This code runs for every puzzle in the DB
        //std::cout << ": " << decode_id(feat.id) << "\n";
        
        // You can also track state outside the lambda
        puzzles_in_check++; });



    }
}