#pragma once

#include <vector>
#include <string>
#include <filesystem>

struct file_struct {
	std::string audio_filename;
	std::string title;
	std::string artist;
	std::string creator;
	std::string difficulty;
	std::string bg_photo_name;

	int preview_time = 0;
	int beatmap_set_id = 0;
	int beatmap_id = 0;

	float hp = 5.0f;
	float cs = 4.0f;
	float od = 8.0f;
	float ar = 9.0f;

	float star_rating = 5.0f;
};

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
				std::filesystem::path relative_path = e.path().parent_path().filename() / e.path().filename();
				r.push_back(relative_path.generic_string());
			}
		}
		return r;
	}

	static void reconstruct_db();
	static void add_to_db();
	static void read_db(std::vector<file_struct>& db);
	static file_struct read_file_metadata(const std::filesystem::path& p);
	static inline std::filesystem::path fs_path;
};