
#include <string_view>
#include "chess/types.h"
#include "chess/file_io.h"
#include "chess/bitboard.h"
#define PROLOG_MODULE "user"
#include "SWI-cpp2.h"


PREDICATE(hello, 1) {
    return A1.unify_integer(0);
}



struct PosCtxt {
    Chess::DBReader::SweepState it;
};

PREDICATE_NONDET(position, 1) {
    auto ctxt = handle.context_unique_ptr<PosCtxt>();

    switch (handle.foreign_control())
    {
    case PL_FIRST_CALL: {
        Chess::Bitboards::init();

        //std::string csv_file = "../data/athousand_sorted.csv";
        std::string csv_file = "data/lichess_db_puzzle.csv";
        std::string db_file = "data/db_out.chess_db";
        Chess::build_csv_parse_and_records(csv_file, db_file, true);

        Chess::DBReader reader(db_file.c_str());

        ctxt.reset(new PosCtxt{ reader.start_sweep() });
    }
        break;
    case PL_REDO:
        break;
    case PL_PRUNED:
        return true;
    default:
        assert(0);
        return false;
    }

    assert(A1.is_variable());

    if (ctxt->it.is_valid())
    {
        A1.unify_integer(0);
        //ctxt->it.advance();
        PL_retry_address(ctxt.release());
    }
    return true;
}
