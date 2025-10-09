#include "raylib.h"
#include <vector>
#include <string>
#include <filesystem>
#include <iostream>
#include "db.hpp";

enum GAME_STATES {
	MAIN_MENU,
	OPTIONS,
	SONG_SELECT_INIT,
	SONG_SELECT,
	GAME,
	SCORE_SCREEN
};

int main()
{	
	std::ios::sync_with_stdio(false);

	auto game_state = MAIN_MENU;
	// INIT SEQUENCE
	db::init();
	
	SetConfigFlags(FLAG_VSYNC_HINT);
	InitWindow(1024, 768, "cppsu!");

	Font font12 = LoadFontEx("resources/aller.ttf", 12, NULL, 0);
	Font font24 = LoadFontEx("resources/aller.ttf", 24, NULL, 0);
	Font font36 = LoadFontEx("resources/aller.ttf", 36, NULL, 0);

	Font font12_b = LoadFontEx("resources/aller_bold.ttf", 12, NULL, 0);
	Font font24_b = LoadFontEx("resources/aller_bold.ttf", 24, NULL, 0);
	Font font36_b = LoadFontEx("resources/aller_bold.ttf", 36, NULL, 0);

	Font font12_l = LoadFontEx("resources/aller_light.ttf", 12, NULL, 0);
	Font font24_l = LoadFontEx("resources/aller_light.ttf", 24, NULL, 0);
	Font font36_l = LoadFontEx("resources/aller_light.ttf", 36, NULL, 0);
	
	std::vector<file_struct> map_list;
	
	auto screen_width = (float)GetScreenWidth();
	auto screen_height = (float)GetScreenHeight();

	db::read_db(map_list);
	
	// Song Select variables clean these up bro
	double current_position = 0.5;
	const int visible = 11;
	const float entry_row_height = 86.0f;
	int map_list_size = (int)map_list.size();
	int selected_map = -1;
	float scroll_speed = 0;
	double map_space_normalized = 1.0 / map_list.size();


	while (!WindowShouldClose())
	{
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
				

				current_position = std::clamp(current_position, -map_space_normalized, 2 * map_space_normalized + 1.0 - visible * map_space_normalized);

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
								if(selected_map != map_list[idx].beatmap_id)
								selected_map = map_list[idx].beatmap_id;
								else {
									// enter game
								}
								std::cout << "Selected map: " << map_list[idx].title << " by " << map_list[idx].artist << " (" << map_list[idx].creator << ") [" << map_list[idx].difficulty << "]\n";
							}
						}
					}
				// draw 11 rows
				for (int i = 0; i < visible; ++i) {
					const auto& m = map_list[base + i];

					float y_origin = -86.0f + 32.0f + i * entry_row_height - (float)(frac * entry_row_height);
					float x_origin = screen_width / 2.0f + abs(screen_height/2 - y_origin)*0.1;

					if (selected_map == m.beatmap_id) {
						x_origin -= 32.0f;
						DrawRectangle(x_origin - 4, y_origin, 640, 80, WHITE);
						DrawTextEx(font24, m.title.c_str(), { x_origin, y_origin }, 24, 0, BLACK);
						DrawTextEx(font36, (m.artist + " // " + m.creator).c_str(), { x_origin, y_origin + 24 }, 18, 0, BLACK);
						DrawTextEx(font36_b, m.difficulty.c_str(), { x_origin, y_origin + 42 }, 18, 0, BLACK);
						continue;
					}
					// if selected map = m_beatmap_set_id mimic normal osu
					DrawRectangle(x_origin - 4, y_origin, 640, 80, ORANGE);
					DrawTextEx(font24, m.title.c_str(), { x_origin, y_origin }, 24, 0, WHITE);
					DrawTextEx(font36, (m.artist + " // " + m.creator).c_str(), { x_origin, y_origin + 24 }, 18, 0, WHITE);
					DrawTextEx(font36_b, m.difficulty.c_str(), { x_origin, y_origin + 42 }, 18, 0, WHITE);
				}

				// DrawText(("current_position: " + std::to_string(current_position)).c_str(), 32, 512, 24, WHITE);

				scroll_speed += IsKeyPressed(KEY_DOWN) * -3.0f + IsKeyPressed(KEY_UP) * 3.0f;

				scroll_speed -= GetMouseWheelMove();
				scroll_speed *= 0.99f;
				current_position += scroll_speed * map_space_normalized * 0.01;
				current_position = std::clamp(current_position, -map_space_normalized, 2*map_space_normalized + 1.0 - visible * (1.0 / std::max(map_list_size, 1)));
				// might wanna reset scroll_speed if we hit the bounds implement if im not lazy lol
			break;
		}
		ClearBackground(DARKGRAY);
		EndDrawing();
	}
	CloseWindow();
	return 0;
}
