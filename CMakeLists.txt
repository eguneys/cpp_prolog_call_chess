cmake_minimum_required(VERSION 3.20)

project(main
   VERSION 0.1.0
   LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#set(DuckDB_USE_STATIC_LIBS TRUE)

#add_subdirectory(duckdb_static)


add_executable(main
   src/create_db.cpp
   src/chess/bitboard.cpp
   src/chess/moves.cpp
   )

include_directories(duckdb_static)
target_link_libraries(main ${CMAKE_CURRENT_SOURCE_DIR}/duckdb_static/libduckdb.so)

#target_compile_options(main PRIVATE /arch:AVX2)
target_compile_options(main PRIVATE -march=native)