#pragma once

#include "globals.hpp"
#include <vector>
#include <string>
#include <filesystem>
//#include <mutex>

class db {
public:
	static void init();
	static inline std::vector<std::string> get_directories(const std::filesystem::path& p)
	{
		std::vector<std::string> r;
		for (auto& p : std::filesystem::recursive_directory_iterator(p))
			if (p.is_directory())
				r.push_back(p.path().filename().string());
		return r;
	}

	static inline std::vector<std::string> get_files(const std::filesystem::path& p) {
		std::vector<std::string> r;
		for (auto& e : std::filesystem::directory_iterator(p)) {
			if (e.path().extension() == ".osu") {
				std::filesystem::path relative_path = e.path().filename();
				r.push_back(relative_path.generic_string());
			}
		}
		return r;
	}

	static inline void write_to_file(std::ofstream& database, file_struct data) {
		database << "[MAP]\t" << data.audio_filename << "\t" << data.creator << "\t" << data.difficulty << "\t" << data.bg_photo_name << "\t" << data.preview_time << "\t" << data.beatmap_id << "\t" << data.hp << "\t" << data.cs << "\t" << data.od << "\t" << data.ar << "\t" << data.slider_multiplier << "\t" << data.slider_tickrate << "\t" << data.star_rating << "\t" << data.min_bpm << "\t" << data.avg_bpm << "\t" << data.max_bpm << "\t" << data.map_length << "\t" << data.circle_count << "\t" << data.slider_count << "\t" << data.spinner_count << "\t" << data.osu_filename << "\n";
	}

	static void reconstruct_db();
	static std::vector<std::string> open_osz_dialog();
	static bool extract_osz_to(const std::string& osz_path_w, const std::filesystem::path& dest);
	static int detect_set_id(const std::filesystem::path& extracted_root, std::string osz_path);
	static bool db_contains_set(std::ifstream& dbin, int set_id);
	static bool finalize_import(int set_id);
	static bool append_set_to_db(int set_id);
	static void update_last_ids();
	static bool add_to_db(std::vector<std::string>& maps_getting_added);
	static bool remove_from_db(int method, int mapID, int setID);
	static void get_last_assigned_id();
	static void read_db(std::vector<file_struct>& db);
	static file_struct read_file_metadata(const std::filesystem::path& p);
	
	static inline std::filesystem::path fs_path;
	static inline int last_assigned_id = 1000000000;
	static inline int last_assigned_map_id = 0;
};