#pragma once
#include <vector>
#include "db.hpp"
#include "raylib.h"

class song_select {
	public:
		static void choose_beatmap(int idx);
		static void enter_game(file_struct map);
		static void init(bool alreadyInitialized);
		static void update();
		static void draw();

		static std::vector<file_struct> map_list;

		static int visible_entries;
		static float entry_row_height;
		static double current_position;
		static float scroll_speed;
	
		static int selected_map_list_index;
		static int map_list_size;
		static int selected_mapset;
		static file_struct selected_map;

		static int y_offset;
		static int max_base;
		static std::filesystem::path loaded_bg_path;
		static std::filesystem::path loaded_audio_path;
};