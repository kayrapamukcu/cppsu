#include "db.hpp"
#include <fstream>
#include <iostream>
#include <array>
#include <unordered_map>
#include "tinyfiledialogs.h"
#include <string>
#include <thread>
#include <miniz.h>
#include <mutex>
#include <unordered_set>
#include "globals.hpp"


void db::init() {
	fs_path = std::filesystem::current_path();
}

void db::reconstruct_db() {
    // Create new file
    auto begin = std::chrono::steady_clock::now();
    int total_maps = 0;
    std::ofstream database(fs_path / "database.db");
    database << "[General]\nFormat: 4\n\n" << "[Data]\n";;

    // Loop over all directories
    std::filesystem::path maps_path = fs_path / "maps";
    if (!std::filesystem::is_directory(maps_path)) {
        std::filesystem::create_directory(maps_path);
		Notice n = { "No beatmaps found. Please import beatmaps and retry.", 5.0f };
		notices.push_back(n);
        return;
    }

    if (std::filesystem::is_empty(maps_path)) {
        Notice n = { "No beatmaps found. Please import beatmaps and retry.", 5.0f };
        notices.push_back(n);
		return;
    }
		
    auto dirs = get_directories(maps_path);

    
    for (auto& d : dirs) {
        auto content = get_files(maps_path / d); // Get all .osu files in directory
        database << "[SET]\t" << d;

        // Read first file to get artist/title
        file_struct data = read_file_metadata(maps_path / d / content[0]);
        database << "\t" << data.title << "\t" << data.artist << "\n";

        for (auto& c : content) { // Loop over all .osu files in directory
            total_maps++;
            file_struct data = read_file_metadata(maps_path / d / c);
            database << "[MAP]\t" << data.audio_filename << "\t" << data.creator << "\t" << data.difficulty << "\t" << data.bg_photo_name << "\t" << data.preview_time << "\t" << data.beatmap_id << "\t" << data.hp << "\t" << data.cs << "\t" << data.od << "\t" << data.ar << "\t" << data.star_rating << "\t" << data.min_bpm << "\t" << data.avg_bpm << "\t" << data.max_bpm << "\t" << data.map_length << "\t" << data.circle_count << "\t" << data.slider_count << "\t" << data.spinner_count << "\n";
        }
    }

    auto end = std::chrono::steady_clock::now();

	notices.push_back(Notice{ "Database reconstructed from " + std::to_string(total_maps) + " maps in " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()) + "ms", 10.0f });
}

std::vector<std::string> open_osz_dialog() {
    const char* filter_patterns[1] = { "*.osz" };
    const char* result = tinyfd_openFileDialog(
        "Import beatmap",
        "",
        1,
        filter_patterns,
        "osu! Beatmap Files",
        1
    );
    if (!result) return {};

	std::vector<std::string> files;
	std::stringstream ss(result);
	std::string item;

    while (std::getline(ss, item, '|')) {
        files.push_back(item);
    }

    return files;
}

bool extract_osz_to(const std::string& osz_path_w, const std::filesystem::path& dest) {
    // Miniz takes UTF-8 path. Convert:
    std::string osz_path_utf8(osz_path_w.begin(), osz_path_w.end());

    mz_zip_archive zip{};
    if (!mz_zip_reader_init_file(&zip, osz_path_utf8.c_str(), 0)) return false;

    const mz_uint num = mz_zip_reader_get_num_files(&zip);
    for (mz_uint i = 0; i < num; ++i) {
        mz_zip_archive_file_stat st{};
        if (!mz_zip_reader_file_stat(&zip, i, &st)) { mz_zip_reader_end(&zip); return false; }
        if (st.m_is_directory) continue;

        std::filesystem::path outPath = dest / std::filesystem::path(st.m_filename);
        std::filesystem::create_directories(outPath.parent_path());

        if (!mz_zip_reader_extract_to_file(&zip, i, outPath.string().c_str(), 0)) {
            mz_zip_reader_end(&zip);
            return false;
        }
    }

    mz_zip_reader_end(&zip);
    return true;
}

int detect_set_id(const std::filesystem::path& extracted_root, std::string osz_path) {
    auto path = std::filesystem::path(osz_path);
    for (auto& e : std::filesystem::recursive_directory_iterator(extracted_root)) {
        if (e.path().extension() == ".osu") {
            auto md = db::read_file_metadata(e.path());
            if (md.beatmap_set_id > 0) return md.beatmap_set_id;
            else  {
                return -1;
            }
        }
    }
    return -1;
}

static bool db_contains_set(std::ifstream& dbin, int set_id) {
    dbin.clear();
    dbin.seekg(0, std::ios::beg);
    std::string needle = "[SET]\t" + std::to_string(set_id);
    std::string line;
    while (std::getline(dbin, line)) {
        if (line.rfind(needle, 0) == 0) return true;
    }
    return false;
}

static bool finalize_import(int set_id) {
    // Move from _import_tmp to maps/
    std::error_code ec;
    const std::filesystem::path final_dir = db::fs_path / "maps" / std::to_string(set_id);
    
    if (set_id == -1) {
		Notice n = { "Could not detect set ID(map file invalid/too old), import failed.", 5.0f };
		notices.push_back(n);
		std::filesystem::remove_all(db::fs_path / "maps/_import_tmp", ec);
		return false;
    }
    std::ifstream dbin(db::fs_path / "database.db");
    if (db_contains_set(dbin, set_id)) {
		Notice n = { "Set " + std::to_string(set_id) + " already exists, import failed.", 5.0f };
		notices.push_back(n);
        std::filesystem::remove_all(db::fs_path / "maps/_import_tmp", ec);
        return false;
    }
    if (std::filesystem::exists(final_dir)) {
        std::filesystem::remove_all(final_dir, ec);
    }
    std::filesystem::rename(db::fs_path / "maps/_import_tmp", final_dir, ec);
    if (ec) {
        Notice n = { "Rename failed (is it cross-volume / locked?): " + final_dir.string() + " (" + ec.message() + ")\n", 7.5f};
        std::filesystem::remove_all(db::fs_path / "maps/_import_tmp", ec);
		return false;
    }
	
    

    std::cout << "Imported set " << set_id << "\n";
    return true;
}

bool db::append_set_to_db(int set_id) {
    std::lock_guard<std::mutex> lk(g_dbFileMutex);

    const auto dbpath = fs_path / "database.db";
    const auto set_path = fs_path / "maps" / std::to_string(set_id);

    if (!std::filesystem::exists(dbpath)) {
        reconstruct_db();
        return true;
    }
    if (!std::filesystem::exists(set_path)) return false;
    auto files = get_files(set_path);
    if (files.empty()) return false;

    file_struct head = read_file_metadata(set_path / files[0]);    

    std::ifstream dbin(dbpath);
    if (!dbin) {
        reconstruct_db();
        return true;
    }

    /*const bool set_already_there = db_contains_set(dbin, set_id);
    std::unordered_set<int> existing_ids;
    if (set_already_there) {
        return false;
    }*/

    // Open DB for append
    std::ofstream dbout(dbpath, std::ios::app);
    if (!dbout) return false;

    //if (!set_already_there) {
        // Write the [SET] header
        dbout << "[SET]\t" << set_id
            << '\t' << head.title
            << '\t' << head.artist << '\n';
    //}

    // Append [MAP] lines
    for (const auto& fn : files) {
        file_struct m = read_file_metadata(set_path / fn);

        dbout << "[MAP]\t" << m.audio_filename << "\t" << m.creator << "\t" << m.difficulty << "\t" << m.bg_photo_name << "\t" << m.preview_time << "\t" << m.beatmap_id << "\t" << m.hp << "\t" << m.cs << "\t" << m.od << "\t" << m.ar << "\t" << m.star_rating << "\t" << m.min_bpm << "\t" << m.avg_bpm << "\t" << m.max_bpm << "\t" << m.map_length << "\t" << m.circle_count << "\t" << m.slider_count << "\t" << m.spinner_count << "\n";
    }

    dbout.flush();
    return true;
}

bool db::add_to_db(std::vector<std::string>& maps_getting_added) {
	auto files = open_osz_dialog();
    if (files.empty()) return false;
    game_state = IMPORTING;
    importing_map = true;
    std::thread([files, &maps_getting_added]() {
        bool failed = false;
        for (auto& f : files) {
            extract_osz_to(f, db::fs_path / "maps/_import_tmp");
            int id = detect_set_id(db::fs_path / "maps/_import_tmp", f);
            bool ok = finalize_import(id); // true if we're actually importing a new set
            if (!failed)
                if(ok) 
                    if (!append_set_to_db(id)) {
                        failed = true;
						std::lock_guard<std::mutex> lk(g_import_msgs_mtx);
                        maps_getting_added.emplace_back(std::string("Failed to import " + std::to_string(id) + " to database! (already exists?)"));
                    }
                    else {
                        std::lock_guard<std::mutex> lk(g_import_msgs_mtx);
                        maps_getting_added.emplace_back(std::to_string(id) + " added successfully!");
                    }
                else {
                    std::lock_guard<std::mutex> lk(g_import_msgs_mtx);
                    maps_getting_added.emplace_back(std::string("Failed to import " + std::to_string(id) + " to database! (already exists/map too old/invalid)"));
                }
        }
        if (failed) reconstruct_db();
        importing_map = false;
		std::lock_guard<std::mutex> lk(g_import_msgs_mtx);
        }).detach();
	return true;
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
    db.clear();
    //try {
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

                to_int(s.substr(a + 1, b - (a + 1)), beatmap_set_id);
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
    /* } catch (const std::exception& e) {
        std::cerr << "Error reading database: " << e.what() << "\n";
        reconstruct_db();
	}*/
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

    int state = 0;

    while (std::getline(f, line)) {
		chomp_cr(line);
        if (line == "[TimingPoints]") {
            state = 1;
            break;
        }
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
        if (!val.empty() && val.front() == ' ') val.remove_prefix(1);

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
        if (state < 1) {
            if (line == "[TimingPoints]") state = 1;
            else continue;
        }

        if (state == 1) {
            chomp_cr(line);
            std::string_view sv = line;
            if (line == "[Colours]") state = 2;
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
