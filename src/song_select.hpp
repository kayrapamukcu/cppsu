#pragma once
#include <vector>
#include "db.hpp"
#include "raylib.h"

class song_select {
	public:
	static void init();
	static void update();
	static void draw();

	static std::vector<file_struct> map_list;

	static constexpr int visible_entries = 11;
	static float entry_row_height;
	static double current_position;
	static float scroll_speed;
	static double map_space_normalized;
	
	static int map_list_size;
	static int selected_mapset;
	static file_struct selected_map;
};