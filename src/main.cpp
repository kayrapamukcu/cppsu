#include "raylib.h"
#include <vector>
#include <string>
#include <filesystem>
#include <iostream>
#include "db.hpp";

enum GAME_STATES {
	MAIN_MENU,
	OPTIONS,
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
	db::reconstruct_db();
	
	SetConfigFlags(FLAG_VSYNC_HINT);
	InitWindow(800, 600, "cppsu!");

	Font font12 = LoadFontEx("resources/aller.ttf", 12, NULL, 0);
	Font font24 = LoadFontEx("resources/aller.ttf", 24, NULL, 0);
	Font font36 = LoadFontEx("resources/aller.ttf", 36, NULL, 0);
	
	std::vector<file_struct> map_list;
	
	auto screen_width = (float)GetScreenWidth();
	auto screen_height = (float)GetScreenHeight();

	double map_space_normalized = 0.1;
	double current_position = 0.5;
	db::read_db(map_list);
	map_space_normalized = 1.0 / map_list.size();

	while (!WindowShouldClose())
	{
		BeginDrawing();
		switch (game_state) {
			case MAIN_MENU:
				DrawTextEx(font36, "Welcome to cppsu!", { 32, 32 }, 36, 0, WHITE);
				DrawTextEx(font24, "Press M to switch to the song select screen!", { 32, 548 }, 24, 0, WHITE);
				if (IsKeyPressed(KEY_M)) game_state = SONG_SELECT;
			break;
			case SONG_SELECT:
				DrawTextEx(font24, "Press B to go back to the main menu!", { 32, 548 }, 24, 0, WHITE);
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
				for (size_t i = 0; i < 9; i++) {
					auto& m = map_list[i+std::clamp(current_position * map_list.size(), 0.0, (double)map_list.size() - 9)];
					DrawTextEx(font24, (std::to_string(i+1) + ". " + m.artist + " - " + m.title + " (" + m.creator + ")" + " [" + m.difficulty + "]").c_str(), {32, (float)32 + (i * 32)}, 24, 0, WHITE);
					DrawText(("current_position: " + std::to_string(current_position)).c_str(), 32, 512, 24, WHITE);
				}

				if (IsKeyPressed(KEY_DOWN)) {
					current_position += map_space_normalized;
					if (current_position > 1.0-9*map_space_normalized) current_position = 1.0 - 9 * map_space_normalized;
				}

				if (IsKeyPressed(KEY_UP)) {
					current_position -= map_space_normalized;
					if (current_position < 0.0) current_position = 0.0;
				}
				
			break;
		}
		ClearBackground(BLACK);
		EndDrawing();
	}
	CloseWindow();
	return 0;
}
