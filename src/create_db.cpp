#include <iostream>
#include "duckdb.hpp"


int main() {
    duckdb::DuckDB db(nullptr);

    duckdb::Connection con(db);

    std::cout << "Done." <<std::endl;

    return 0;
}