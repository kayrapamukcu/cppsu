#pragma once
#include <vector>
#include <tuple>
#include <array>
#include "db.hpp"
#include "raylib.h"

struct score_entry {
	uint32_t score;
	std::array<char, 16> name;
	std::array<char, 32> time_buf;
	std::array<bool, (int)MODS::COUNT> mod_array;
	std::string score_text;
	std::string acc_text;
	std::string mod_text;
	std::array<float, 3> text_widths;
	Vector2 hover_text_size;

	std::string hover_text;
	std::string filename;
	RANKS rank;
};

class song_select {
	public:
		static void choose_beatmap(int idx);
		static void enter_game(file_struct map);
		static void init(bool alreadyInitialized);
		static void update();
		static void draw();
		static void callback_choose_random_map(int);
		static void callback_change_menu(int menu);
		static void callback_change_mods(int mod);
		static void callback_draw_mods(button_callback& b);
		

		static file_struct selected_map;
		static int selected_map_list_index;
		
	private:

		static void load_score_list(const std::filesystem::path path, const std::vector<std::string> scores);

		static inline std::vector<score_entry> score_list;
		static inline std::vector<file_struct> map_list;

		static int visible_entries;
		static float entry_row_height;
		static double current_position;
		static float scroll_speed;
		
		static int map_list_size;
		static int selected_mapset;
		
		static int y_offset;
		static int max_base;
		static std::filesystem::path loaded_bg_path;
		static std::filesystem::path loaded_audio_path;
		static inline float score_multiplier = 1.0f;
		enum class S_SUBMENU {
			NONE,
			MODS,
			BEATMAP,
		};
		static S_SUBMENU submenu;

		static inline std::array<bool, (int)MODS::COUNT> selected_mods;
		static inline char score_str[8];
		static inline std::string selected_mods_string;
		static inline std::vector<button_callback> buttons_submenu_none;
		static inline std::vector<button_callback> buttons_submenu_mods;
		static inline std::vector<button_callback> buttons_submenu_beatmap;
};