#pragma once

#include <iostream>
#include <functional>

#include <future>
#include <thread>

#include <filesystem>
#include <span>
#include <stdexcept>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <fstream>
#include <vector>

#include "bitboard.h"
#include "types.h"
#include "bitset.h"
#include "moves.h"

namespace Chess {

struct Segment {
    u64 count;
    u64 records_offset;
    u64 features_offset;
    u64 bitsets_offset;
};


struct DBHeader {
    u32 magic = 0x5a7a70;
    u32 version = 2;
    u64 total_count = 0;
    u32 segment_count = 0;
    u32 words_per_bitset = 0; // relative to the segment size
    Segment segments[1024];
};

struct PositionRecord
{
    Bitboard pieces[6]; // 48 bytes

    Bitboard white_occ; // 8

    u16 best_move;
    
    u16 state;

    u32 reserved;
};

static_assert(sizeof(PositionRecord) == 64);
static_assert(std::is_trivially_copyable_v<PositionRecord>);

struct PositionFeatures {
    u64 id;
    u64 white_attacks;
    u64 black_attacks;
};

struct PositionBitsets {
    Bitset is_check;
};



enum class Feature : u32 {
    KingInCheck,
    HangingPiece,
    BishopAttack,
    Count
};

bool check_logic(Feature feature, const PositionRecord& record);

constexpr size_t FEATURE_COUNT = static_cast<size_t>(Feature::Count);


class DBWriter {
    std::ofstream out;
    DBHeader header;
    
    // The "Working Set" in RAM
    std::vector<PositionRecord> chunk_records;
    std::vector<PositionFeatures> chunk_features;
    std::vector<uint64_t> chunk_bitsets;

    const size_t CHUNK_THRESHOLD = 100000;

public:
    DBWriter(const std::filesystem::path& path) {
        out.open(path, std::ios::binary);
        // Placeholder for header (we rewrite it at the end)
        out.write(reinterpret_cast<char*>(&header), sizeof(header));
    }

    void add(const PositionRecord& rec, const PositionFeatures& feat) {
        chunk_records.push_back(rec);
        chunk_features.push_back(feat);

        if (chunk_records.size() >= CHUNK_THRESHOLD) {
            flush_segment();
        }
    }

    void align() {
        uint64_t pos = out.tellp();
        uint64_t padding = (8 - (pos % 8)) % 8;
        for (uint64_t i = 0; i < padding; ++i) out.put(0);
    }

    void flush_segment() {
    if (chunk_records.empty()) return;

    uint64_t N = chunk_records.size();
    uint64_t words_per_feature = (N + 63) >> 6;
    
    // Allocate a zeroed block for all features in this segment
    // Size: FEATURE_COUNT * words_per_feature
    std::vector<uint64_t> bitset_block(FEATURE_COUNT * words_per_feature, 0);

    for (size_t f_idx = 0; f_idx < FEATURE_COUNT; ++f_idx) {
        Feature feature_type = static_cast<Feature>(f_idx);

        Bitset current_feature_bits = Bitset(&bitset_block[f_idx * words_per_feature], words_per_feature);

        // Populate the bitsets for this chunk
        for (uint64_t i = 0; i < N; ++i) {
            // Example: Logic to determine which features are active for chunk_records[i]
            // This is where you'd call your 'is_position_king_in_check' logic
            if (check_logic(feature_type, chunk_records[i])) {
                current_feature_bits.set(i);
            }
        }
    }

    // --- File Writing ---
    align();
    Segment& s = header.segments[header.segment_count++];
    s.count = N;
    
    // 1. Write Records
    s.records_offset = out.tellp();
    out.write((char*)chunk_records.data(), N * sizeof(PositionRecord));
    
    // 2. Write Features
    align();
    s.features_offset = out.tellp();
    out.write((char*)chunk_features.data(), N * sizeof(PositionFeatures));
    
    // 3. Write Bitset Block
    align();
    s.bitsets_offset = out.tellp();
    out.write((char*)bitset_block.data(), bitset_block.size() * sizeof(uint64_t));

    header.total_count += N;
    chunk_records.clear(); 
    chunk_features.clear();
}


    void finalize() {
        flush_segment(); // Write remaining data
        out.seekp(0);
        out.write(reinterpret_cast<char*>(&header), sizeof(header));
        out.close();
    }
};

struct PuzzleResult {
    const PositionRecord& record;
    const PositionFeatures& feature;
};

class DBReader {
    uint8_t* base_ptr;
    size_t file_size;
    const DBHeader* header;

public:
    DBReader(const char* path) {
        int fd = open(path, O_RDONLY);
        struct stat st;
        fstat(fd, &st);
        file_size = st.st_size;
        base_ptr = (uint8_t*)mmap(nullptr, file_size, PROT_READ, MAP_SHARED, fd, 0);
        header = (DBHeader*)base_ptr;
    }

    template <typename Func>
    void sweep(Func callback) const {
        for (uint32_t s = 0; s < header->segment_count; ++s) {
            const Segment& seg = header->segments[s];
            
            auto* recs = (PositionRecord*)(base_ptr + seg.records_offset);
            auto* feats = (PositionFeatures*)(base_ptr + seg.features_offset);

            // This is the high-speed sweep
            for (uint64_t i = 0; i < seg.count; ++i) {
                const auto& r = recs[i];
                const auto& f = feats[i];
                // Process...
                callback(recs[i], feats[i]);
            }
        }
    }



    /*
    reader.sweep_parallel([](const PositionRecord& rec, 
                          const PositionFeatures& feat, 
                          auto get_bitset, 
                          uint64_t local_idx) {
    
    // Retrieve your custom Bitset class for the specific feature
    auto check_bits = get_bitset(Feature::KingInCheck);

    if (check_bits.test(local_idx)) {
        // Handle King in Check logic...
    }
});
    */

    template <typename F>
    void sweep_parallel(F callback) const {
        uint32_t num_segments = header->segment_count;
        uint32_t num_cores = std::thread::hardware_concurrency();
        
        // Safety check: don't spawn more threads than segments
        uint32_t num_workers = std::min(num_segments, num_cores);
        
        std::vector<std::future<void>> tasks;
        const uint8_t* base = static_cast<const uint8_t*>(base_ptr);
    
        for (uint32_t t = 0; t < num_workers; ++t) {
            tasks.push_back(std::async(std::launch::async, [this, t, num_workers, num_segments, base, &callback]() {
                // Each thread handles segments: t, t + num_workers, t + 2*num_workers, etc.
                for (uint32_t i = t; i < num_segments; i += num_workers) {
                    const Segment& s = header->segments[i];
                    
                    auto* recs = reinterpret_cast<const PositionRecord*>(base + s.records_offset);
                    auto* feats = reinterpret_cast<const PositionFeatures*>(base + s.features_offset);
                    
                    size_t words_per_feature = (s.count + 63) >> 6;
                    uint64_t* bitset_block_start = (uint64_t*)(base + s.bitsets_offset);
    
                    auto get_bitset = [&](int feature_id) {
                        uint64_t* feature_data = bitset_block_start + (feature_id * words_per_feature);
                        return Chess::Bitset(feature_data, s.count);
                    };
    
                    for (uint64_t j = 0; j < s.count; ++j) {
                        callback(recs[j], feats[j], get_bitset, j);
                    }
                }
            }));
        }
    
        for (auto& task : tasks) task.get();
    }


    PuzzleResult get_puzzle(uint64_t global_index) const {
        uint64_t accumulated = 0;

        for (uint32_t s = 0; s < header->segment_count; ++s) {
            const Segment& seg = header->segments[s];
            
            // Check if the global index falls within this segment
            if (global_index < accumulated + seg.count) {
                uint64_t local_index = global_index - accumulated;

                auto* recs = (const PositionRecord*)(base_ptr + seg.records_offset);
                auto* feats = (const PositionFeatures*)(base_ptr + seg.features_offset);

                return { recs[local_index], feats[local_index] };
            }
            accumulated += seg.count;
        }
        throw std::out_of_range("Global index exceeds total puzzle count");
    }

    // Helper to check a specific bit (feature) for a global index
    bool test_feature(uint64_t global_index, int feature_id) const {
        uint64_t accumulated = 0;
        for (uint32_t s = 0; s < header->segment_count; ++s) {
            const Segment& seg = header->segments[s];
            if (global_index < accumulated + seg.count) {
                uint64_t local_index = global_index - accumulated;
                
                // Calculate words per bitset for this specific segment
                uint64_t words_per_bitset = (seg.count + 63) >> 6;
                const uint64_t* bitset_start = (const uint64_t*)(base_ptr + seg.bitsets_offset);
                
                // Navigate to the specific feature's bitset, then the specific word
                const uint64_t* feature_bitset = bitset_start + (feature_id * words_per_bitset);
                return (feature_bitset[local_index >> 6] >> (local_index & 63)) & 1;
            }
            accumulated += seg.count;
        }
        return false;
    }


    ~DBReader() { munmap(base_ptr, file_size); }



    class SweepState
    {
    public:
        SweepState(const DBReader &parent)
            : base_ptr(parent.base_ptr), header(parent.header),
              current_segment(0), current_index(0)
        {
            if (header && header->segment_count > 0)
            {
                load_current_segment();
            }
        }

        bool is_valid() const
        {
            return header && current_segment < header->segment_count;
        }

        void advance()
        {
            if (!is_valid())
                return;

            ++current_index;
            if (current_index >= header->segments[current_segment].count)
            {
                ++current_segment;
                current_index = 0;
                if (is_valid())
                {
                    load_current_segment();
                }
            }
            else
            {
                ++current_rec;
                ++current_feat;
            }
        }

        const PositionRecord &get_record() const
        {
            if (!is_valid())
                throw std::runtime_error("Invalid state");
            return *current_rec;
        }

        const PositionFeatures &get_features() const
        {
            if (!is_valid())
                throw std::runtime_error("Invalid state");
            return *current_feat;
        }

        std::pair<const PositionRecord &, const PositionFeatures &> get() const
        {
            return {get_record(), get_features()};
        }

    private:
        void load_current_segment()
        {
            const auto &seg = header->segments[current_segment];
            current_rec = (const PositionRecord *)(base_ptr + seg.records_offset);
            current_feat = (const PositionFeatures *)(base_ptr + seg.features_offset);
        }

        const uint8_t *base_ptr;
        const DBHeader *header;
        uint32_t current_segment;
        uint64_t current_index;
        const PositionRecord *current_rec = nullptr;
        const PositionFeatures *current_feat = nullptr;
    };

    SweepState start_sweep() const
    {
        return SweepState(*this);
    }
};




#include <filesystem>
#include <span>
#include <stdexcept>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

class MMapFile
{
public:

    explicit MMapFile(const std::filesystem::path& path)
    {
        fd = open(path.c_str(), O_RDONLY);
        if (fd == -1)
            throw std::runtime_error("open failed");

        struct stat st{};
        if (fstat(fd, &st) == -1)
            throw std::runtime_error("fstat failed");

        size_ = st.st_size;

        data_ = mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, fd, 0);
        if (data_ == MAP_FAILED)
            throw std::runtime_error("mmap failed");
    }

    ~MMapFile()
    {
        if (data_)
            munmap(data_, size_);

        if (fd != -1)
            close(fd);
    }

    std::span<const char> bytes() const
    {
        return {reinterpret_cast<const char*>(data_), size_};
    }

private:
    int fd{-1};
    void* data_{nullptr};
    size_t size_{0};
};




struct CsvRow
{
    std::string_view id;
    std::string_view fen;
    std::string_view moves;
};



class CsvReader
{
public:

    explicit CsvReader(std::span<const char> buffer)
        : p(buffer.data()),
          end(buffer.data() + buffer.size()) {}

    bool next(CsvRow& row)
    {
        if (p >= end)
            return false;

        const char* id_begin = p;

        while (*p != ',') p++;
        const char* id_end = p++;

        const char* fen_begin = p;

        while (*p != ',') p++;
        const char* fen_end = p++;

        const char* move_begin = p;

        while (p < end && *p != '\n' && *p != '\r') p++;
        const char* move_end = p;

        // handle CRLF
        if (p < end && *p == '\r') p++;
        if (p < end && *p == '\n') p++;

        row.id    = std::string_view(id_begin, id_end - id_begin);
        row.fen   = std::string_view(fen_begin, fen_end - fen_begin);
        row.moves = std::string_view(move_begin, move_end - move_begin);

        return true;
    }

private:

    const char* p;
    const char* end;
};

void apply_move(PositionRecord& record, Move move);
void castle(PositionRecord& record, Square from, Square to);
void parse_fen(const std::string_view& fen, PositionRecord& rec);
u16 encode_move(const Move& move);
u64 encode_id(const std::string_view& id);
std::string decode_id(u64 encoded);

void build_features(const PositionRecord& rec, PositionFeatures& feat);

void build_csv_parse_and_records(const std::string& csv_file, const std::string& db_file, bool check_exists);

inline bool empty(const PositionRecord &record, Square to) {
    return ~(record.pieces[0] |
             record.pieces[1] |
             record.pieces[2] |
             record.pieces[3] |
             record.pieces[4] |
             record.pieces[5]) &
           to;
}
inline Piece piece_on(const PositionRecord& record, Square from) {

    for (size_t pt = Pawn; pt <= King; pt++) {
        if (record.pieces[pt - 1] & from) {
            Color color = record.white_occ & from ? White : Black;
            return make_piece(color, PieceType(pt));
        }
    }
    return No_Piece;
}


inline void put_piece(PositionRecord& record, Piece pc, Square sq) {
    record.pieces[typeof_piece(pc) - 1] |= sq;
    if (color_of(pc) == White) {
        record.white_occ |= sq;
    }
}

inline void put_side_to_move(PositionRecord& record, Color c) {
    record.state = c;
}

inline void remove_piece(PositionRecord& record, Square from) {
    Piece piece = piece_on(record, from);
    assert(piece != No_Piece);
    record.pieces[typeof_piece(piece) - 1] ^= from;
    if (color_of(piece) == White) {
        record.white_occ ^= from;
    }
}
inline void swap_piece(PositionRecord& record, Square s, Piece pc) {
    remove_piece(record, s);
    put_piece(record, pc, s);
}
inline void move_piece(PositionRecord& record, Square from, Square to) {
    Piece piece = piece_on(record, from);
    assert(piece != No_Piece);

    Bitboard fromTo = from | to;

    record.pieces[typeof_piece(piece) - 1] ^= fromTo;

    if (color_of(piece) == White) {
        record.white_occ ^= fromTo;
    }
}

inline Color side_to_move(const PositionRecord& record) {
    return Color(record.state);
}
}