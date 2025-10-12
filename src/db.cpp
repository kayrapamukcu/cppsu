#include "db.hpp"
#include <fstream>
#include <iostream>
#include <array>
#include <unordered_map>

void db::init() {
	fs_path = std::filesystem::current_path();
}

void db::reconstruct_db() {
    // Create new file
    auto begin = std::chrono::steady_clock::now();
    int total_maps = 0;
    std::ofstream database(fs_path / "database.db");
    database << "[General]\nFormat: 4\n\n";

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
            database << "[MAP]\t" << data.audio_filename << "\t" << data.creator << "\t" << data.difficulty << "\t" << data.bg_photo_name << "\t" << data.preview_time << "\t" /* << data.beatmap_set_id << "\t"*/ << data.beatmap_id << "\t" << data.hp << "\t" << data.cs << "\t" << data.od << "\t" << data.ar << "\t" << data.star_rating << "\t" << data.min_bpm << "\t" << data.avg_bpm << "\t" << data.max_bpm << "\t" << data.map_length << "\t" << data.circle_count << "\t" << data.slider_count << "\t" << data.spinner_count << "\n";
        }
    }

    auto end = std::chrono::steady_clock::now();
    std::cout << "DB reconstructed in " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "ms\n";
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
        if (format_version != 4) { std::cout << "Database format not supported, reconstructing...\n"; reconstruct_db(); return; }
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
            std::array<std::string_view, 18> parts{};
            int index = 0;
            size_t start = 0;
            size_t pos = content.find('\t', start);
            start = pos + 1;
            while (index < 17) {
                size_t pos = content.find('\t', start);
                parts[index++] = content.substr(start, pos - start);
                start = pos + 1;
            }
            parts[17] = content.substr(start);

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
			to_float(parts[11], f_tmp); rec.min_bpm = f_tmp;
			to_float(parts[12], f_tmp); rec.avg_bpm = f_tmp;
			to_float(parts[13], f_tmp); rec.max_bpm = f_tmp;
			to_int(parts[14], i_tmp); rec.map_length = (uint16_t)i_tmp;
			to_int(parts[15], i_tmp); rec.circle_count = (uint16_t)i_tmp;
			to_int(parts[16], i_tmp); rec.slider_count = (uint16_t)i_tmp;
			to_int(parts[17], i_tmp); rec.spinner_count = (uint16_t)i_tmp;



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
    
    int timing_point = 0;
    float bpm_infile = 250.0f;
    float bpm = 120.0f;
    bool hitColours = false;
	std::vector<std::pair<int, float>> timing_points;
    while(std::getline(f, line)) {
        if (!hitColours) {
            chomp_cr(line);
            std::string_view sv = line;
            if (line == "[Colours]") hitColours = true;
            if (line == "[HitObjects]") break;
            size_t first_comma = sv.find(',');
            size_t second_comma = sv.find(',', first_comma + 1);
			if (first_comma == std::string_view::npos || second_comma == std::string_view::npos) continue;

            to_float(sv.substr(first_comma + 1, second_comma - (first_comma + 1)), bpm_infile);
            if (bpm_infile > 0) {
                to_int(sv.substr(0, first_comma), timing_point);
                bpm = 60000.0f / std::stof(std::string(sv.substr(first_comma + 1, second_comma - (first_comma + 1))));
                timing_points.push_back({ timing_point, bpm });
            }
        }
        else {
            if (line == "[HitObjects]") break;
        }
        
    }

    int time_first_ms = 0;
    int time_last_ms = 0;

    auto first_pos = f.tellg();
    std::getline(f, line);

	size_t first_comma = line.find(',');
	size_t second_comma = line.find(',', first_comma + 1);
	size_t third_comma = line.find(',', second_comma + 1);

    to_int(line.substr(second_comma + 1, third_comma - (second_comma + 1)), time_first_ms);
    f.seekg(first_pos);

    while(std::getline(f, line)) {
        chomp_cr(line);
        if (line.empty()) continue;
        std::string_view sv = line;
        size_t first_comma = sv.find(',');
        size_t second_comma = sv.find(',', first_comma + 1);
        size_t third_comma = sv.find(',', second_comma + 1);
        size_t fourth_comma = sv.find(',', third_comma + 1);

        to_int(sv.substr(second_comma + 1, third_comma - (second_comma + 1)), time_last_ms);

        std::string_view type_field = sv.substr(third_comma + 1, fourth_comma - (third_comma + 1));
        int type = 0;
        to_int(type_field, type);
        if (type & 1) md.circle_count++;
        else if (type & 2) md.slider_count++;
        else if (type & 8) md.spinner_count++;
	}

	md.map_length = (uint16_t)((time_last_ms - time_first_ms) / 1000);

    if (!timing_points.empty() && time_last_ms > time_first_ms) {
        std::unordered_map<float, double> dur_by_bpm;
        float min_bpm = 99999999.0f;
        float max_bpm = 0.0f;

        for (size_t i = 0; i < timing_points.size(); ++i) {
            const int sec_start = timing_points[i].first;
            const float sec_bpm = timing_points[i].second;
            const int sec_end = (i + 1 < timing_points.size()) ? timing_points[i + 1].first : std::max(sec_start, time_last_ms);

            const int start = std::max(sec_start, time_first_ms);
            const int end = std::min(sec_end, time_last_ms);

            dur_by_bpm[sec_bpm] += double(end - start);

            if (sec_bpm < min_bpm) min_bpm = sec_bpm;
            if (sec_bpm > max_bpm) max_bpm = sec_bpm;
        }

        // Pick BPM with max duration
        float dominant_bpm = 0.0f;
        double best_ms = -1.0;
        for (const auto& [bpm_val, dur] : dur_by_bpm) {
            if (dur > best_ms) { best_ms = dur; dominant_bpm = bpm_val; }
        }

        // If nothing accumulated (e.g., all sections outside range), fall back to first positive BPM
        if (best_ms < 0) {
			md.avg_bpm = timing_points[0].second;
			md.max_bpm = md.min_bpm = md.avg_bpm;
        }

        md.avg_bpm = dominant_bpm;
        md.min_bpm = min_bpm;
        md.max_bpm = max_bpm;
    }
    else {
        md.avg_bpm = 120.0f;
        md.min_bpm = md.max_bpm = md.avg_bpm;
    }

    return md;
}
