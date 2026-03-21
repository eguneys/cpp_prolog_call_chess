#include "file_io.h"
#include "moves.h"
#include "bitboard.h"

namespace Chess {



bool check_logic(Feature feature, const PositionRecord& record) {
    return false;
}

void build_features(const PositionRecord& rec, PositionFeatures& feat) {

}

void apply_move(PositionRecord& record, Move move)
{

    put_side_to_move(record, ~side_to_move(record));

    Square from = move.from_sq();
    Square to = move.to_sq();

    bool captured = !empty(record, to);

    if (captured)
    {
        Piece pc = piece_on(record, from);
        remove_piece(record, from);
        swap_piece(record, to, pc);
    }
    else
    {
        if (distance(from, to) == 2) {
            Piece pc = piece_on(record, from);
            if (typeof_piece(pc) == King) {
               castle(record, from, to); 
               return;
            }
        }

        move_piece(record, from, to);
    }
}


void castle(PositionRecord& record, Square from, Square to) {
    move_piece(record, from, to);
    switch (to) {
        case G8: move_piece(record, H8, F8); break;
        case G1: move_piece(record, H1, F1); break;
        case C8: move_piece(record, A8, D8); break;
        case C1: move_piece(record, A1, D1); break;
    }
}

constexpr Piece char_to_piece(char c)
{
    switch (c)
    {
    case 'P':
        return White_Pawn;
    case 'N':
        return White_Knight;
    case 'B':
        return White_Bishop;
    case 'R':
        return White_Rook;
    case 'Q':
        return White_Queen;
    case 'K':
        return White_King;
    case 'p':
        return Black_Pawn;
    case 'n':
        return Black_Knight;
    case 'b':
        return Black_Bishop;
    case 'r':
        return Black_Rook;
    case 'q':
        return Black_Queen;
    case 'k':
        return Black_King;
    default:
        return No_Piece;
    }
}

void parse_fen(const std::string_view &FEN, PositionRecord& rec)
{
    const char *ptr = FEN.data();
    const char *const end = ptr + FEN.size();

    Square sq = A8;

    // Fast path: parse until space
    while (ptr < end)
    {
        const char c = *ptr;
        if (c == ' ')
            break;
        ++ptr;

        if (c >= '1' && c <= '8')
        {
            sq = (Square)((int)sq + (c - '0'));
        }
        else if (c == '/')
        {
            sq = (Square)((int)sq + (2 * Down));
        }
        else
        {
            const Piece piece = char_to_piece(static_cast<unsigned char>(c));
            if (piece != No_Piece)
            {
                put_piece(rec, piece, sq);
                ++sq;
            }
        }
    }

    // Skip spaces and parse side to move
    while (ptr < end && *ptr == ' ')
    {
        ++ptr;
    }
    if (ptr < end)
    {
        put_side_to_move(rec, (*ptr == 'w') ? White : Black);
        ++ptr;
    }
}

u64 encode_ascii_id(const std::string_view &id)
{
    if (id.length() > 8)
    {
        throw std::invalid_argument("ID must be at most 8 characters");
    }

    u64 result = 0;
    for (size_t i = 0; i < id.length(); i++)
    {
        result = (result << 8) | static_cast<u8>(id[i]);
    }
    return result;
}

std::string decode_ascii_id(u64 encoded)
{
    std::string result;
    result.reserve(8);
    
    // Copy the bytes directly
    for (int i = 0; i < 8; i++)
    {
        char c = static_cast<char>((encoded >> (56 - i * 8)) & 0xFF);
        if (c != '\0')  // Skip null padding
        {
            result.push_back(c);
        }
    }
    
    return result;
}

u64 encode_id(const std::string_view& id) {
    return encode_ascii_id(id);
}


u16 encode_move(const Move& move) {
    return move.raw();
}



std::vector<std::string_view> split_moves(std::string_view moves_str) {
    std::vector<std::string_view> moves;
    
    size_t split_end = moves_str.find(',');
    size_t start = 0;
    size_t end = moves_str.find(' ');
    
    while (end < split_end) {
        // Add the move (from start to end)
        if (end > start) {  // Skip empty strings (consecutive spaces)
            moves.push_back(moves_str.substr(start, end - start));
        }
        start = end + 1;
        end = moves_str.find(' ', start);
    }
    
    // Add the last move (or only move if no spaces)
    if (start < moves_str.size()) {
        moves.push_back(moves_str.substr(start, moves_str.find(',', start) - start));
    }
    
    return moves;
}


// Generate all positions along the move sequence
std::vector<std::pair<PositionRecord, PositionFeatures>> generate_position_sequence(
    const u64 id,
    const std::string_view& initial_fen, 
    const std::string_view& moves_str) 
{
    std::vector<std::pair<PositionRecord, PositionFeatures>> sequence;
    
    // Start with initial position
    PositionRecord current_rec{};
    parse_fen(initial_fen, current_rec);
    //current_rec.id = id;
    
    // Add initial position with its features
    //PositionFeatures initial_features{};
    //build_features(current_rec, initial_features);
    //sequence.emplace_back(current_rec, initial_features);
    
    // Split and process each move
    auto moves = split_moves(moves_str);
    
    size_t i = 0;

    for (const std::string_view& move_str : moves) {
        Move move = Move::parse_uci(move_str);
        // Apply the move to get new position
        PositionRecord next_rec = current_rec; // Copy current state
        apply_move(next_rec, move); // You'll need this function
        
        // Set move-specific fields
        next_rec.best_move = encode_move(move);
        //next_rec.id = id;
        
        // Build features for this position
        PositionFeatures features{id};
        build_features(next_rec, features);
        
        // Add to sequence
        if (i++ % 2 == 0)
        {
            sequence.emplace_back(next_rec, features);
        }

        // Update current for next iteration
        current_rec = std::move(next_rec);
    }
    
    return sequence;
}



void build_csv_parse_and_records2(const std::string& csv_file, const std::string& db_file) {
    MMapFile file(csv_file);
    CsvReader csv(file.bytes());

    CsvRow row;


    DBWriter builder(db_file);

    while (csv.next(row))
    {
        u64 id = encode_id(row.id);

        {
            // Usage
            auto position_sequence = generate_position_sequence(id, row.fen, row.moves);

            // Now you can use all positions in the sequence
            for (const auto &[rec, features] : position_sequence)
            {
                // Do something with each position and its features
                // e.g., train on all positions, analyze each position, etc.
                builder.add(rec, features);
            }
        }
    }

    builder.finalize();
}

bool should_create_db(const std::filesystem::path &path, uint32_t expected_version)
{
    // Returns false if file exists and version matches (don't create), 
    // true if file doesn't exist or version doesn't match (safe to create)
    
    if (!std::filesystem::exists(path)) {
        return true;  // File doesn't exist, safe to create
    }
    
    // File exists, try to read the version
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        // Can't open existing file, assume it's corrupted and create new one
        return true;
    }
    
    uint32_t file_version = 0;
    // Read the first u32 (assuming it's a magic number) and skip it
    uint32_t magic;
    if (!file.read(reinterpret_cast<char*>(&magic), sizeof(magic))) {
        return true;  // Failed to read magic, create new file
    }
    
    // Read the second u32 which is the version
    if (!file.read(reinterpret_cast<char*>(&file_version), sizeof(file_version))) {
        return true;  // Failed to read version, create new file
    }

    // Check if the version matches
    return file_version != expected_version;  // Return true if versions don't match (create new)

}


void build_csv_parse_and_records(const std::string& csv_file, const std::string& db_file, bool check_exists) {
    DBHeader header{};
    if (check_exists && !should_create_db(db_file, header.version)) {
        return;
    }

    std::cout << "Building DB version: " << header.version << std::endl;
    build_csv_parse_and_records2(csv_file, db_file);
}


}