#pragma once
#include <vector>
#include <tuple>
#include "db.hpp"
#include "raylib.h"


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
		static inline constexpr std::array<std::tuple<MODS, std::string_view, float, std::string_view>, (int)MODS::COUNT> mod_info {
			// mod, abbr., mult., name
			std::tuple{ MODS::AT, "AT", 1.0f, "Auto"},
			std::tuple{ MODS::SO, "SO", 0.9f, "SpunOut"},
			std::tuple{ MODS::EZ, "EZ", 0.5f, "Easy"},
			std::tuple{ MODS::NF, "NF", 0.5f, "NoFail"},
			std::tuple{ MODS::HD, "HD", 1.06f, "Hidden"},
			std::tuple{ MODS::HT, "HT", 0.30f, "HalfTime"},
			std::tuple{ MODS::DT, "DT", 1.12f, "DoubleTime"},
			std::tuple{ MODS::NC, "NC", 1.12f, "Nightcore"},
			std::tuple{ MODS::HR, "HR", 1.06f, "HardRock"},
			std::tuple{ MODS::SD, "SD", 1.0f, "SuddenDeath"},
			std::tuple{ MODS::PF, "PF", 1.0f, "Perfect"},
			std::tuple{ MODS::AP, "AP", 0.0f, "AutoPilot"},
			std::tuple{ MODS::RX, "RX", 0.0f, "Relax"},
			std::tuple{ MODS::FL, "FL", 1.12f, "Flashlight"}
		};

		static std::vector<file_struct> map_list;

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