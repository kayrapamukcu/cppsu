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
std::filesystem::path song_select::loaded_bg_path;
std::filesystem::path song_select::loaded_audio_path;

void song_select::choose_beatmap(int idx) {
	auto& before_tex = map_list[idx].bg_photo_name;
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
		loaded_audio_path = audio_path;
	}
	else selected_map = map_list[idx];
	if(IsMusicStreamPlaying(music) == false) {
		PlayMusicStream(music);
		SeekMusicStream(music, selected_map.preview_time / 1000.0f);
	}
	selected_map_list_index = idx;
	selected_mapset = map_list[idx].beatmap_set_id;


	std::filesystem::path bg_path = db::fs_path / "maps" / std::to_string(selected_map.beatmap_set_id) / selected_map.bg_photo_name;
	if (loaded_bg_path != bg_path) {
		UnloadTexture(background.tex);
		if (bg_path.filename() == "") {
			bg_path = db::fs_path / "resources" / "textures" / "default_bg.jpg";
		}
		background = LoadTextureCompat(bg_path.string().c_str());
	}

	std::filesystem::path audio_path = db::fs_path / "maps" / std::to_string(selected_map.beatmap_set_id) / selected_map.audio_filename;

	if (loaded_audio_path != audio_path) {
		UnloadMusicStreamFromRam(music);
		music = LoadMusicStreamFromRam(audio_path.string().c_str());
		if(!music.ctxData) {
			std::cout << "Failed to load music: " << audio_path << "\n";
			game_state = MAIN_MENU;
			return;
		}
		loaded_audio_path = audio_path;
		PlayMusicStream(music);
		SeekMusicStream(music, selected_map.preview_time / 1000.0f);
	}

	loaded_bg_path = bg_path;
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
		visible_entries = 11;
		max_base = std::max(0, map_list_size - visible_entries);
		y_offset = 0;
		if (map_list_size < 11) {
			y_offset = (int)((10 - map_list_size) * entry_row_height / 2.0f);
		}
	}

	if(IsMusicValid(music)) SetMusicPitch(music, 1.0f);
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

	int base = (int)std::floor(current_position);

	float frac = (float)current_position - (float)base;
	float y_origin, x_origin;

	// check for clicks
	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
		for (int i = 0; i < visible_entries; ++i) {
			y_origin = screen_height_ratio * (y_offset + 32.0f + i * entry_row_height - frac * entry_row_height);
			x_origin = screen_width_ratio * (512.0f + abs(screen_height / 2 - y_origin) * 0.1f);
			if (GetMouseY() >= y_origin && GetMouseY() <= y_origin + entry_row_height * screen_height_ratio && GetMouseX() > x_origin) {
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
	if (IsKeyPressed(KEY_ENTER)) enter_game(selected_map);
}

void song_select::draw() {
	if (map_list.empty()) return;
	DrawTextureCompatPro(background, { 0,0, screen_width, screen_height }, WHITE);

	double pos_idx = std::clamp(current_position, 0.0, (double)max_base);

	int base = (int)std::floor(current_position);

	float frac = (float)current_position - (float)base;
	float y_origin, x_origin;

	for (int i = 0; i < visible_entries; ++i) {
		int n = base + i;
		if (n > map_list_size - 1) break;
		if (n < 0) continue;
		const auto& m = map_list[n];

		y_origin = screen_height_ratio * (y_offset + 32.0f + i * entry_row_height - frac * entry_row_height);
		x_origin = screen_width_ratio * (512.0f + abs(screen_height / 2 - y_origin) * 0.1f);

		x_origin = floorf(x_origin);
		y_origin = floorf(y_origin);

		if (selected_map.beatmap_id == m.beatmap_id) {
			x_origin -= 48.0f * screen_width_ratio;
			DrawRectangle(x_origin - 4*screen_width_ratio, y_origin, 640 * screen_width_ratio, 80 * screen_height_ratio, WHITE);
			DrawTextEx(aller_r, m.title.c_str(), { x_origin, y_origin }, 24 * screen_height_ratio, 0, BLACK);
			DrawTextEx(aller_r, (m.artist + " // " + m.creator).c_str(), { x_origin, y_origin + 24 * screen_height_ratio }, 18 * screen_height_ratio, 0, BLACK);
			DrawTextEx(aller_b, m.difficulty.c_str(), { x_origin, y_origin + 42 * screen_height_ratio }, 18 * screen_height_ratio, 0, BLACK);
			continue;
		}
		else if (selected_mapset == m.beatmap_set_id) {
			x_origin -= 32.0f * screen_width_ratio;
			DrawRectangle(x_origin - 4*screen_width_ratio, y_origin, 640 * screen_width_ratio, 80 * screen_height_ratio, Color{ 25, 86, 209, 255 });
			DrawTextEx(aller_r, m.title.c_str(), { x_origin, y_origin }, 24 * screen_height_ratio, 0, WHITE);
			DrawTextEx(aller_r, (m.artist + " // " + m.creator).c_str(), { x_origin, y_origin + 24 * screen_height_ratio }, 18 * screen_height_ratio, 0, WHITE);
			DrawTextEx(aller_b, m.difficulty.c_str(), { x_origin, y_origin + 42 * screen_height_ratio }, 18 * screen_height_ratio, 0, WHITE);
			continue;
		}
		DrawRectangle(x_origin - 4*screen_width_ratio, y_origin, 640 * screen_width_ratio, 80 * screen_height_ratio, ORANGE);
		DrawTextEx(aller_r, m.title.c_str(), { x_origin, y_origin }, 24 * screen_height_ratio, 0, WHITE);
		DrawTextEx(aller_r, (m.artist + " // " + m.creator).c_str(), { x_origin, y_origin + 24 * screen_height_ratio }, 18 * screen_height_ratio, 0, WHITE);
		DrawTextEx(aller_b, m.difficulty.c_str(), { x_origin, y_origin + 42 * screen_height_ratio }, 18 * screen_height_ratio, 0, WHITE);
	}

	DrawTexturePro(song_select_top_bar, { 0, 0, (float)song_select_top_bar.width, (float)song_select_top_bar.height }, { 0, 0, screen_width, screen_height / 5.4f }, { 0, 0 }, 0.0f, WHITE);
	DrawTextEx(aller_l, (selected_map.artist + " - " + selected_map.title + " [" + selected_map.difficulty + "]").c_str(), { 60*screen_scale, 4*screen_scale }, 36*screen_scale, 0, WHITE);
	DrawTextEx(aller_l, ("Mapped by " + selected_map.creator).c_str(), { 60*screen_scale, 36*screen_scale }, 24*screen_scale, 0, WHITE);

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
	DrawTextExScaled(aller_b, stats_1.c_str(), { 4, 64 }, 24, 0, WHITE);

	std::string stats_2 = std::format(
		"Circles: {}  Sliders: {}  Spinners: {}",
		selected_map.circle_count,
		selected_map.slider_count,
		selected_map.spinner_count
	);
	DrawTextExScaled(aller_l, stats_2.c_str(), { 4, 88 }, 24, 0, WHITE);

	std::string stats_3 = std::format(
		"CS:{}  AR:{}  OD:{}  HP:{}  Stars:{}",
		format_floats(selected_map.cs),
		format_floats(selected_map.ar),
		format_floats(selected_map.od),
		format_floats(selected_map.hp),
		format_floats(selected_map.star_rating)
	);
	DrawTextExScaled(aller_r, stats_3.c_str(), { 4,112 }, 18, 0, WHITE);
}