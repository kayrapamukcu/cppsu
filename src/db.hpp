#pragma once

#include "globals.hpp"
#include <vector>
#include <string>
#include <filesystem>
//#include <mutex>

class db {
public:
	static void init();
	static std::vector<std::string> get_directories(const std::filesystem::path& p)
	{
		std::vector<std::string> r;
		for (auto& p : std::filesystem::recursive_directory_iterator(p))
			if (p.is_directory())
				r.push_back(p.path().filename().string());
		return r;
	}

	static std::vector<std::string> get_files(const std::filesystem::path& p) {
		std::vector<std::string> r;
		for (auto& e : std::filesystem::directory_iterator(p)) {
			if (e.path().extension() == ".osu") {
				std::filesystem::path relative_path = e.path().filename();
				r.push_back(relative_path.generic_string());
			}
		}
		return r;
	}

	static void reconstruct_db();
	static bool append_set_to_db(int set_id);
	static bool add_to_db(std::vector<std::string>& maps_getting_added);
	static bool remove_from_db(int method, int mapID, int setID);
	static void read_db(std::vector<file_struct>& db);
	static file_struct read_file_metadata(const std::filesystem::path& p);

	static inline std::filesystem::path fs_path;
};