#include "raylib.h"
#include <vector>
#include <string>
#include <filesystem>
#include <iostream>
#include <format>
#include "db.hpp";
#include "rlgl.h"

bool isNPOTSupported = false;
float screen_width = 0;
float screen_height = 0;

enum GAME_STATES {
	MAIN_MENU,
	OPTIONS,
	SONG_SELECT_INIT,
	SONG_SELECT,
	GAME,
	SCORE_SCREEN
};

static inline std::string format_floats(float v) {
	std::string s = std::format("{:.2f}", v);
	while (!s.empty() && s.back() == '0') s.pop_back();
	if (!s.empty() && s.back() == '.') s.pop_back();
	return s;
}

static std::string format_length(uint16_t length) {
	int minutes = length / 60;
	int seconds = length % 60;
	return std::string(minutes < 10 ? "0" : "") + std::to_string(minutes) + ":" + (seconds < 10 ? "0" : "") + std::to_string(seconds);
}

static inline bool IsPOT(int v) { return v > 0 && (v & (v - 1)) == 0; }
static inline int NextPOT(int v) { if (v <= 1) return 1; v--; v |= v >> 1; v |= v >> 2; v |= v >> 4; v |= v >> 8; v |= v >> 16; return v + 1; }

struct TexWithSrc {
	Texture2D tex{};
	Rectangle src{ 0,0,0,0 };  // original content area inside the texture
};

TexWithSrc LoadTextureCompat(const std::string& path) {
	TexWithSrc out;
	Image img = LoadImage(path.c_str());
	const int origW = img.width, origH = img.height;

	const bool isNPOT = !IsPOT(img.width) || !IsPOT(img.height);

	if (isNPOT && !isNPOTSupported) {
		ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
		ImageResizeCanvas(&img, NextPOT(img.width), NextPOT(img.height), 0, 0, BLANK);
	}

	out.tex = LoadTextureFromImage(img);
	UnloadImage(img);

	out.src = { 0, 0, (float)origW, (float)origH };

	return out;
}

void DrawTextureCompatPro(const TexWithSrc& t, Rectangle dst, Color tint) {
	DrawTexturePro(t.tex, t.src, dst, { 0,0 }, 0.0f, tint);
}

void DrawTextureCompat(const TexWithSrc& t, Vector2 pos, Color tint) {
	DrawTexturePro(t.tex, t.src, { pos.x, pos.y, t.src.width, t.src.height }, { 0,0 }, 0.0f, tint);
}

int main()
{	
	std::ios::sync_with_stdio(false);
	
	auto game_state = MAIN_MENU;
	// INIT SEQUENCE
	db::init();
	
	InitWindow(1024, 768, "cppsu!");
	InitAudioDevice();
	std::cout << "Version: " << rlGetVersion() << "\n";

	if(rlIsNPOTSupported() == 0) {
		std::cout << "No NPOT!!\n";
		isNPOTSupported = false;
	} else {
		std::cout << "NPOT supported!\n";
		isNPOTSupported = false;
	}

	Font font12 = LoadFontEx("resources/aller.ttf", 12, NULL, 0);
	Font font24 = LoadFontEx("resources/aller.ttf", 24, NULL, 0);
	Font font36 = LoadFontEx("resources/aller.ttf", 36, NULL, 0);

	Font font12_b = LoadFontEx("resources/aller_bold.ttf", 12, NULL, 0);
	Font font24_b = LoadFontEx("resources/aller_bold.ttf", 24, NULL, 0);
	Font font36_b = LoadFontEx("resources/aller_bold.ttf", 36, NULL, 0);

	Font font12_l = LoadFontEx("resources/aller_light.ttf", 12, NULL, 0);
	Font font24_l = LoadFontEx("resources/aller_light.ttf", 24, NULL, 0);
	Font font36_l = LoadFontEx("resources/aller_light.ttf", 36, NULL, 0);

	TexWithSrc song_select_top_bar = LoadTextureCompat("resources/mode-osu-small.png");
	TexWithSrc background = LoadTextureCompat((db::fs_path / "resources" / "default_bg.jpg").string().c_str());

	Music music = LoadMusicStream("resources/mus_menu.ogg");


	std::vector<file_struct> map_list;
	
	screen_width = (float)GetScreenWidth();
	screen_height = (float)GetScreenHeight();

	db::reconstruct_db();
	db::read_db(map_list);
	SetTargetFPS(1000);
	// Song Select variables clean these up bro
	double current_position = 0.5;
	const int visible = 11;
	const float entry_row_height = 86.0f;
	int map_list_size = (int)map_list.size();
	// int selected_map = -1;
	int selected_mapset = -1;
	float scroll_speed = 0;
	double map_space_normalized = 1.0 / map_list.size();

	file_struct selected_map = map_list[0];
	selected_mapset = selected_map.beatmap_set_id;

	UpdateMusicStream(music);
	PlayMusicStream(music);

	while (!WindowShouldClose())
	{
		UpdateMusicStream(music);
		BeginDrawing();
		switch (game_state) {
			case MAIN_MENU:
				DrawTextEx(font36, "Welcome to cppsu!", { 32, 32 }, 36, 0, WHITE);
				DrawTextEx(font24, "Press M to switch to the song select screen!", { 32, screen_height - 64 }, 24, 0, WHITE);
				if (IsKeyPressed(KEY_M)) game_state = SONG_SELECT_INIT;
			break;
			case SONG_SELECT_INIT:
				// set up variables, check for db validity/existence

				game_state = SONG_SELECT;
			break;
			case SONG_SELECT:
				DrawTextEx(font24, "Press B to go back to the main menu!", { 32, screen_height - 64}, 24, 0, WHITE);
				if (IsKeyPressed(KEY_B)) game_state = MAIN_MENU;
				if (IsKeyPressed(KEY_R)) {
					map_list.clear();
					db::read_db(map_list);
					std::cout << "Database read!\n";
					map_space_normalized = map_list.size() / 1.0;
				}
				if (IsKeyPressed(KEY_G)) {
					db::reconstruct_db();
				}
				if (IsKeyPressed(KEY_F4)) {
					ToggleFullscreen();
				}
				
				scroll_speed += IsKeyPressed(KEY_DOWN) * 3.0f + IsKeyPressed(KEY_UP) * -3.0f;
				float dt = GetFrameTime();
				scroll_speed -= GetMouseWheelMove();
				scroll_speed *= std::max(0.0f, 1.0f - dt);

				// multiply by dt so the motion per second is consistent
				current_position += scroll_speed * map_space_normalized * 0.01f * (dt * 120.0f);
				current_position = std::clamp(current_position, -3 * map_space_normalized, 2 * map_space_normalized + 1.0 - visible * map_space_normalized);

				double logical = current_position * map_list_size;
				int base = std::clamp((int)std::floor(logical), 0, std::max(map_list_size - visible, 0));
				double frac = logical - base; // [0,1)

				// check for clicks
				if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
					for (int i = 0; i < visible; ++i) {
						float y_origin = -86.0f + 32.0f + i * entry_row_height - (float)(frac * entry_row_height);
						float x_origin = 16 + screen_width / 2.0f + abs(screen_height / 2 - y_origin) * 0.1;
						if (GetMouseY() >= y_origin && GetMouseY() <= y_origin + 80 && GetMouseX() > x_origin) {
							int idx = base + i;
							if (idx >= 0 && idx < map_list_size) {
								if (selected_map.beatmap_id != map_list[idx].beatmap_id) { // Selected new map!
									if (selected_mapset != map_list[idx].beatmap_set_id) {
										selected_map = map_list[idx];
										std::filesystem::path audio_path = db::fs_path / "maps" / std::to_string(selected_map.beatmap_set_id) / selected_map.audio_filename;
										UnloadMusicStream(music);
										music = LoadMusicStream(audio_path.string().c_str());
										PlayMusicStream(music);
										SeekMusicStream(music, selected_map.preview_time / 1000.0f);
									} else selected_map = map_list[idx];
									UnloadTexture(background.tex);
									std::filesystem::path bg_path = db::fs_path / "maps" / std::to_string(selected_map.beatmap_set_id) / selected_map.bg_photo_name;
									background = LoadTextureCompat(bg_path.string().c_str());
								}
								
								else {
									// enter game
								}
								selected_mapset = map_list[idx].beatmap_set_id;
								
								std::cout << "Selected map: " << map_list[idx].title << " by " << map_list[idx].artist << " (" << map_list[idx].creator << ") [" << map_list[idx].difficulty << "]\n";
							}
						}
					}
				
				DrawTextureCompatPro(background, { 0,0, screen_width, screen_height }, WHITE);
				// draw 11 rows
				for (int i = 0; i < visible; ++i) {
					const auto& m = map_list[base + i];

					float y_origin = -86.0f + 32.0f + i * entry_row_height - (float)(frac * entry_row_height);
					float x_origin = screen_width / 2.0f + abs(screen_height/2 - y_origin)*0.1;

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
						DrawRectangle(x_origin - 4, y_origin, 640, 80, Color{25, 86, 209, 255});
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

				// DrawText(("current_position: " + std::to_string(current_position)).c_str(), 32, 512, 24, WHITE);
				// might wanna reset scroll_speed if we hit the bounds implement if im not lazy lol

				DrawTextureCompat(song_select_top_bar, {0,0}, WHITE);
				DrawTextEx(font36, (selected_map.artist + " - " + selected_map.title + " [" + selected_map.difficulty + "]").c_str(), {4, 4}, 36, 0, WHITE);
				DrawTextEx(font24, ("Mapped by " + selected_map.creator).c_str(), { 4, 40 }, 24, 0, WHITE);

				std::string stats_1;
				if(selected_map.min_bpm == selected_map.max_bpm)
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

			break;
		}
		ClearBackground(DARKGRAY);
		EndDrawing();
	}
	CloseWindow();
	return 0;
}
