#include "song_select.hpp"
#include "raylib.h"
#include <iostream>
#include "globals.hpp"
#include <format>
#include <rlgl.h>
#include "ingame.hpp"

float song_select::entry_row_height = 86.0f;
double song_select::current_position = 0.5;
float song_select::scroll_speed = 0.0f;
int song_select::visible_entries = 11;
std::vector<file_struct> song_select::map_list;
int song_select::map_list_size = 0;
int song_select::selected_mapset = -999;
int song_select::selected_map_list_index = 0;
file_struct song_select::selected_map = file_struct();
int song_select::y_offset = 0;
int song_select::max_base = 0;

void song_select::choose_beatmap(int idx) {
	if (selected_mapset != map_list[idx].beatmap_set_id) {
		selected_map = map_list[idx];
		std::filesystem::path audio_path = db::fs_path / "maps" / std::to_string(selected_map.beatmap_set_id) / selected_map.audio_filename;
		UnloadMusicStreamFromRam(music);
		music = LoadMusicStreamFromRam(audio_path.string().c_str());
		if(!music.ctxData) {
			std::cout << "Failed to load music: " << audio_path << "\n";
			game_state = MAIN_MENU;
			return;
		}
	}
	else selected_map = map_list[idx];
	if(IsMusicStreamPlaying(music) == false) {
		PlayMusicStream(music);
		SeekMusicStream(music, selected_map.preview_time / 1000.0f);
	}
	selected_map_list_index = idx;
	selected_mapset = map_list[idx].beatmap_set_id;
	UnloadTexture(background.tex);
	std::filesystem::path bg_path = db::fs_path / "maps" / std::to_string(selected_map.beatmap_set_id) / selected_map.bg_photo_name;
	background = LoadTextureCompat(bg_path.string().c_str());
}

void song_select::enter_game(file_struct map) {
	scroll_speed = 0.0f;
	StopMusicStream(music);
	game_state = INGAME;
	g_ingame = new ingame(map);
}

void song_select::init(bool alreadyInitialized) {
	if (!alreadyInitialized) {
		db::read_db(map_list);
		if (map_list.empty()) {
			std::cout << "No beatmaps found. Please import beatmaps and retry.\n";
			Notice n;
			n.text = "No beatmaps found. Please import beatmaps and retry.";
			n.time_left = 5.0f;
			notices.push_back(n);

			game_state = MAIN_MENU;
			return;
		}
		selected_map = map_list[0];
		selected_mapset = -999;
		map_list_size = (int)map_list.size();
		//visible_entries = std::min(11, map_list_size);
		visible_entries = 11;
		max_base = std::max(0, map_list_size - visible_entries);

		y_offset = 0;
		if (map_list_size < 11) {
			y_offset = (10 - map_list_size) * entry_row_height / 2;
		}
	}
	game_state = SONG_SELECT;
	choose_beatmap(selected_map_list_index);
}
void song_select::update() {
	if (IsKeyPressed(KEY_B)) game_state = MAIN_MENU;
	
	float dt = GetFrameTime();
	scroll_speed += IsKeyPressed(KEY_DOWN) * 3.0f + IsKeyPressed(KEY_UP) * -3.0f;
	scroll_speed -= GetMouseWheelMove();
	scroll_speed *= std::max(0.0f, 1.0f - dt);

	current_position += scroll_speed * 0.05 * (dt * 120.0f);
	constexpr double extra_space = 3.0f;
	if(map_list_size > 6)
	current_position = std::clamp(current_position, -extra_space, (double)max_base + extra_space);
	else
	current_position = std::clamp(current_position, 0.0, (double)max_base);

	double pos_idx = std::clamp(current_position, 0.0, (double)max_base);

	int base = (int)std::floor(current_position);//pos_idx);

	float frac = current_position - base;
	float y_origin, x_origin;

	// check for clicks
	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
		for (int i = 0; i < visible_entries; ++i) {
			y_origin = y_offset + 32.0f + i * entry_row_height - frac * entry_row_height;
			x_origin = -24.0f + screen_width / 2.0f + abs(screen_height / 2 - y_origin) * 0.1;
			if (GetMouseY() >= y_origin && GetMouseY() <= y_origin + 86 && GetMouseX() > x_origin) {
				int idx = (int)base + i;
				std::cout << "Clicked on entry " << idx << "\n";
				if (idx >= 0 && idx < map_list_size) {
					if (selected_map.beatmap_id != map_list[idx].beatmap_id) { // Selected new map!
						choose_beatmap(idx);
						break;
					}
					else {
						enter_game(selected_map);
						return;
				}
			}
		}
	}
}

void song_select::draw() {
	if (map_list.empty()) return;
	DrawTextureCompatPro(background, { 0,0, screen_width, screen_height }, WHITE);

	double pos_idx = std::clamp(current_position, 0.0, (double)max_base);

	int base = (int)std::floor(current_position);

	float frac = current_position - base;
	float y_origin, x_origin;

	for (int i = 0; i < visible_entries; ++i) {
		int n = base + i;
		if (n > map_list_size - 1) break;
		if (n < 0) continue;
		const auto& m = map_list[n];

		y_origin = y_offset + 32.0f + i * entry_row_height - frac * entry_row_height;
		x_origin = screen_width / 2.0f + abs(screen_height / 2 - y_origin) * 0.1;


		if (selected_map.beatmap_id == m.beatmap_id) {
			x_origin -= 48.0f;
			DrawRectangle(x_origin - 4, y_origin, 640, 80, WHITE);
			DrawTextEx(font24, m.title.c_str(), { x_origin, y_origin }, 24, 0, BLACK);
			DrawTextEx(font36, (m.artist + " // " + m.creator).c_str(), { x_origin, y_origin + 24 }, 18, 0, BLACK);
			DrawTextEx(font36_b, m.difficulty.c_str(), { x_origin, y_origin + 42 }, 18, 0, BLACK);
			continue;
		}
		else if (selected_mapset == m.beatmap_set_id) {
			x_origin -= 32.0f;
			DrawRectangle(x_origin - 4, y_origin, 640, 80, Color{ 25, 86, 209, 255 });
			DrawTextEx(font24, m.title.c_str(), { x_origin, y_origin }, 24, 0, WHITE);
			DrawTextEx(font36, (m.artist + " // " + m.creator).c_str(), { x_origin, y_origin + 24 }, 18, 0, WHITE);
			DrawTextEx(font36_b, m.difficulty.c_str(), { x_origin, y_origin + 42 }, 18, 0, WHITE);
			continue;
		}
		DrawRectangle(x_origin - 4, y_origin, 640, 80, ORANGE);
		DrawTextEx(font24, m.title.c_str(), { x_origin, y_origin }, 24, 0, WHITE);
		DrawTextEx(font36, (m.artist + " // " + m.creator).c_str(), { x_origin, y_origin + 24 }, 18, 0, WHITE);
		DrawTextEx(font36_b, m.difficulty.c_str(), { x_origin, y_origin + 42 }, 18, 0, WHITE);
	}

	DrawTextureCompat(song_select_top_bar, { 0,0 }, WHITE);
	DrawTextEx(font36, (selected_map.artist + " - " + selected_map.title + " [" + selected_map.difficulty + "]").c_str(), { 4, 4 }, 36, 0, WHITE);
	DrawTextEx(font24, ("Mapped by " + selected_map.creator).c_str(), { 4, 40 }, 24, 0, WHITE);

	std::string stats_1;
	if (selected_map.min_bpm == selected_map.max_bpm)
		stats_1 = std::format(
			"Length: {}  BPM: {}  Objects: {}",
			format_length(selected_map.map_length),
			format_floats(selected_map.avg_bpm),
			selected_map.circle_count + selected_map.slider_count + selected_map.spinner_count
		);
	else
		stats_1 = std::format(
			"Length: {}  BPM: {}-{} ({})  Objects: {}",
			format_length(selected_map.map_length),
			format_floats(selected_map.min_bpm),
			format_floats(selected_map.max_bpm),
			format_floats(selected_map.avg_bpm),
			selected_map.circle_count + selected_map.slider_count + selected_map.spinner_count
		);
	DrawTextEx(font24_b, stats_1.c_str(), { 4, 64 }, 24, 0, WHITE);

	std::string stats_2 = std::format(
		"Circles: {}  Sliders: {}  Spinners: {}",
		selected_map.circle_count,
		selected_map.slider_count,
		selected_map.spinner_count
	);
	DrawTextEx(font24, stats_2.c_str(), { 4, 88 }, 24, 0, WHITE);

	std::string stats_3 = std::format(
		"CS:{}  AR:{}  OD:{}  HP:{}  Stars:{}",
		format_floats(selected_map.cs),
		format_floats(selected_map.ar),
		format_floats(selected_map.od),
		format_floats(selected_map.hp),
		format_floats(selected_map.star_rating)
	);
	DrawTextEx(font36_b, stats_3.c_str(), { 4,112 }, 18, 0, WHITE);

	DrawText(std::to_string(current_position).c_str(), 20, 256, 32, WHITE);
}