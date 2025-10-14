#pragma once
#include <vector>
#include "db.hpp"
#include "raylib.h"

class song_select {
	public:
		static void choose_beatmap(int idx);
		static void init();
		static void update();
		static void draw();

		static std::vector<file_struct> map_list;

		static int visible_entries;
		static float entry_row_height;
		static double current_position;
		static float scroll_speed;
	
		static int map_list_size;
		static int selected_mapset;
		static file_struct selected_map;

		static int y_offset;
		static int max_base;
};