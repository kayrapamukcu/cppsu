#include "db.hpp"
#include <fstream>
#include <iostream>
#include <array>

void db::init() {
	fs_path = std::filesystem::current_path();
}

void db::reconstruct_db() {
    // Create new file
    auto begin = std::chrono::steady_clock::now();
    int total_maps = 0;
    std::ofstream database(fs_path / "database.db");
    database << "[General]\nFormat: 3\n\n";

    // Loop over all directories
    std::filesystem::path maps_path = fs_path / "maps";
    auto dirs = get_directories(maps_path);

    database << "[Data]\n";
    for (auto& d : dirs) {
        auto content = get_files(maps_path / d); // Get all .osu files in directory
        database << "[SET]\t" << d;

        // Read first file to get artist/title
        file_struct data = read_file_metadata(maps_path / content[0]);
        database << "\t" << data.title << "\t" << data.artist << "\n";

        for (auto& c : content) { // Loop over all .osu files in directory
            total_maps++;
            file_struct data = read_file_metadata(maps_path / c);
            database << "[MAP]\t" << data.audio_filename << "\t" << data.creator << "\t" << data.difficulty << "\t" << data.bg_photo_name << "\t" << data.preview_time << "\t" /* << data.beatmap_set_id << "\t"*/ << data.beatmap_id << "\t" << data.hp << "\t" << data.cs << "\t" << data.od << "\t" << data.ar << "\t" << data.star_rating << "\n";
        }
    }

    auto end = std::chrono::steady_clock::now();
    std::cout << "DB reconstructed in " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "us\n";
}



void db::add_to_db() {

}

static inline void chomp_cr(std::string& s) {
    // CHOMP
    if (!s.empty() && s.back() == '\r') s.pop_back();
}

static inline void to_int(std::string_view s, int& out) {
    std::from_chars(s.data(), s.data() + s.size(), out, 10);
}

static inline void to_float(std::string_view s, float& out) {
    std::from_chars(s.data(), s.data() + s.size(), out);
}


void db::read_db(std::vector<file_struct>& db) {
    auto begin = std::chrono::steady_clock::now();
    std::ifstream in(fs_path / "database.db");
    if (!in) { std::cout << "No database found, reconstructing...\n"; reconstruct_db(); return; }

    std::string line;
    int to_reserve = 0;
    // Header
    if (!std::getline(in, line)) { reconstruct_db(); return; } // [General]
    if (!std::getline(in, line)) { reconstruct_db(); return; } // Format: x
    chomp_cr(line);
    if (!line.starts_with("Format: ")) { reconstruct_db(); return; }
    int format_version = 0;
    {
        std::string_view v = std::string_view(line).substr(8);
        to_int(v, format_version);
        if (format_version != 3) { std::cout << "Database format not supported, reconstructing...\n"; reconstruct_db(); return; }
    }

    std::string cur_title, cur_artist;
    int beatmap_set_id = 0;

    while (std::getline(in, line)) {
        chomp_cr(line);
        std::string_view s = line;

        if (s.starts_with("[SET]")) {
            size_t a = s.find('\t');
            size_t b = s.find('\t', a + 1);
            size_t c = s.find('\t', b + 1);

            to_int(s.substr(a+1, b-(a+1)),beatmap_set_id);
            cur_title = std::string(s.substr(b + 1, c - (b + 1)));
            cur_artist = std::string(s.substr(c + 1));
            continue;
        }

        if (s.starts_with("[MAP]")) {
            std::string_view content = s.substr(5);
            std::array<std::string_view, 11> parts{};
            int index = 0;
            size_t start = 0;
            size_t pos = content.find('\t', start);
            start = pos + 1;
            while (index < 10) {
                size_t pos = content.find('\t', start);
                parts[index++] = content.substr(start, pos - start);
                start = pos + 1;
            }
            parts[10] = content.substr(start);

            file_struct rec{};
            rec.audio_filename = std::string(parts[0]);
            rec.title = cur_title;
            rec.artist = cur_artist;
			rec.beatmap_set_id = beatmap_set_id;
            rec.creator = std::string(parts[1]);
            rec.difficulty = std::string(parts[2]);
            rec.bg_photo_name = std::string(parts[3]);

            int i_tmp;
            float f_tmp;

            to_int(parts[4], i_tmp); rec.preview_time = i_tmp;     
            to_int(parts[5], i_tmp); rec.beatmap_id = i_tmp;     
            to_float(parts[6], f_tmp); rec.hp = f_tmp;     
            to_float(parts[7], f_tmp); rec.cs = f_tmp;     
            to_float(parts[8], f_tmp); rec.od = f_tmp;     
            to_float(parts[9], f_tmp); rec.ar = f_tmp;     
            to_float(parts[10], f_tmp); rec.star_rating = f_tmp;     

            db.push_back(std::move(rec));
            continue;
        }
    }
	auto end = std::chrono::steady_clock::now();
	std::cout << "DB read in " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "us\n";
}

file_struct db::read_file_metadata(const std::filesystem::path& p) {
    file_struct md;
    std::ifstream f(p);
    if (!f) return md;

    struct Handler {
        std::string_view key;
        void (*set)(file_struct&, std::string_view);
    };

    static constexpr std::array<Handler, 13> handlerList = { {
        {"AudioFilename",      [](auto& m, std::string_view v) { m.audio_filename = std::string(v); }},
        {"Title",       [](auto& m, std::string_view v) { m.title = std::string(v); }},
        {"Artist",      [](auto& m, std::string_view v) { m.artist = std::string(v); }},
        {"Creator",            [](auto& m, std::string_view v) { m.creator = std::string(v); }},
        {"Version",            [](auto& m, std::string_view v) { m.difficulty = std::string(v); }},
        // {"Background",         [](auto& m, std::string_view v) { m.bg_photo_name = std::string(v); }},
        {"PreviewTime",        [](auto& m, std::string_view v) { int x; to_int(v,x);   m.preview_time = x; }},
        {"BeatmapSetID",       [](auto& m, std::string_view v) { int x; to_int(v,x);   m.beatmap_set_id = x; }},
        {"BeatmapID",          [](auto& m, std::string_view v) { int x; to_int(v,x);   m.beatmap_id = x; }},
        {"HPDrainRate",        [](auto& m, std::string_view v) { float x; to_float(v,x); m.hp = x; }},
        {"CircleSize",         [](auto& m, std::string_view v) { float x; to_float(v,x); m.cs = x; }},
        {"OverallDifficulty",  [](auto& m, std::string_view v) { float x; to_float(v,x); m.od = x; }},
        {"ApproachRate",       [](auto& m, std::string_view v) { float x; to_float(v,x); m.ar = x; }},
        {"StarRating",         [](auto& m, std::string_view v) { float x; to_float(v,x); m.star_rating = x; }},
    } };

    std::string line;
    while (std::getline(f, line)) {
		chomp_cr(line);
        if (line == "[TimingPoints]") break;
        std::string_view s = line;

        
        if (line == ("[Events]")) {
            std::getline(f, line);
            std::getline(f, line);
            auto pos = line.find('"');
            line = line.substr(pos+1);
            line = line.substr(0, line.find('"'));
            md.bg_photo_name = line;
            break;
        }

        auto pos = s.find(':');
        if (pos == std::string_view::npos) continue;
        std::string_view key = s.substr(0, pos);
        std::string_view val = s.substr(pos+1);
        if (val.front() == ' ') val.remove_prefix(1);

        for (const auto& h : handlerList) {
            if (key == h.key) { h.set(md, val); break; }
        }
    }
    return md;
}
