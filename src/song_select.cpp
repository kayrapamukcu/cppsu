#include "song_select.hpp"
#include "raylib.h"
#include <iostream>
#include "globals.hpp"
#include <format>
#include <rlgl.h>

float song_select::entry_row_height = 86.0f;
double song_select::current_position = 0.5;
float song_select::scroll_speed = 0.0f;
double song_select::map_space_normalized = 0.1;
int song_select::visible_entries = 11;
std::vector<file_struct> song_select::map_list;
int song_select::map_list_size = 0;
int song_select::selected_mapset = -999;
file_struct song_select::selected_map = file_struct();

void song_select::choose_beatmap(int idx) {
	if (selected_mapset != map_list[idx].beatmap_set_id) {
		selected_map = map_list[idx];
		std::filesystem::path audio_path = db::fs_path / "maps" / std::to_string(selected_map.beatmap_set_id) / selected_map.audio_filename;
		UnloadMusicStream(music);
		music = LoadMusicStream(audio_path.string().c_str());
		if(!music.ctxData) {
			std::cout << "Failed to load music: " << audio_path << "\n";
			game_state = MAIN_MENU;
			return;
		}
		PlayMusicStream(music);
		SeekMusicStream(music, selected_map.preview_time / 1000.0f);
	}
	else selected_map = map_list[idx];

	selected_mapset = map_list[idx].beatmap_set_id;
	UnloadTexture(background.tex);
	std::filesystem::path bg_path = db::fs_path / "maps" / std::to_string(selected_map.beatmap_set_id) / selected_map.bg_photo_name;
	background = LoadTextureCompat(bg_path.string().c_str());
}

void song_select::init() {
	db::read_db(map_list);
	if (map_list.empty()) {
		std::cout << "No beatmaps found. Please import beatmaps and retry.\n";
		game_state = MAIN_MENU;
		return;
	}
	selected_map = map_list[0];
	selected_mapset = -999;
	map_list_size = (int)map_list.size();
	map_space_normalized = 1.0 / (std::max(11, map_list_size));
	visible_entries = std::min(11, map_list_size);
	game_state = SONG_SELECT; 
	choose_beatmap(0);
}
void song_select::update() {
	if (IsKeyPressed(KEY_B)) game_state = MAIN_MENU;
	if (IsKeyPressed(KEY_F4)) {
		ToggleFullscreen();
	}
	float dt = GetFrameTime();
	scroll_speed += IsKeyPressed(KEY_DOWN) * 3.0f + IsKeyPressed(KEY_UP) * -3.0f;
	scroll_speed -= GetMouseWheelMove();
	scroll_speed *= std::max(0.0f, 1.0f - dt);

	current_position += scroll_speed * map_space_normalized * 0.01f * (dt * 120.0f);
	current_position = std::clamp(current_position, -4 * map_space_normalized, 0.85);

	double logical = current_position * map_list_size;
	int base = std::clamp((int)std::floor(logical), 0, std::max(map_list_size - visible_entries, 0));
	double frac = logical - base; // [0,1)

	// check for clicks
	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
		for (int i = 0; i < visible_entries; ++i) {
			float y_origin = -86.0f + 32.0f + i * entry_row_height - (float)(frac * entry_row_height);
			float x_origin = screen_width / 2.0f + abs(screen_height / 2 - y_origin) * 0.1;
			if (GetMouseY() >= y_origin && GetMouseY() <= y_origin + 80 && GetMouseX() > x_origin) {
				int idx = base + i;
				if (idx >= 0 && idx < map_list_size) {
					if (selected_map.beatmap_id != map_list[idx].beatmap_id) { // Selected new map!
						choose_beatmap(idx);
					}
					else {
						// enter game
					}
				}
			}
		}
}

void song_select::draw() {
	if (map_list.empty()) return;
	DrawTextureCompatPro(background, { 0,0, screen_width, screen_height }, WHITE);
	
	// draw 11 rows
	double logical = current_position * map_list_size;
	int base = std::clamp((int)std::floor(logical), 0, std::max(map_list_size - visible_entries, 0));
	double frac = logical - base;
	float y_origin, x_origin;


	for (int i = 0; i < visible_entries; ++i) {
		const auto& m = map_list[base + i];

		y_origin = -86.0f + 32.0f + i * entry_row_height - (float)(frac * entry_row_height);
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