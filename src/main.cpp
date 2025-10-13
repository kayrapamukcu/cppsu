#include "raylib.h"
#include "rlgl.h"
#include <vector>
#include <string>
#include <filesystem>
#include <iostream>
#include <format>
#include "db.hpp";
#include "globals.hpp"
#include "song_select.hpp"

int main()
{	
	std::ios::sync_with_stdio(false);

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
		isNPOTSupported = true;
	}

	font12 = LoadFontEx("resources/aller.ttf", 12, NULL, 0);
	font24 = LoadFontEx("resources/aller.ttf", 24, NULL, 0);
	font36 = LoadFontEx("resources/aller.ttf", 36, NULL, 0);

	font12_b = LoadFontEx("resources/aller_bold.ttf", 12, NULL, 0);
	font24_b = LoadFontEx("resources/aller_bold.ttf", 24, NULL, 0);
	font36_b = LoadFontEx("resources/aller_bold.ttf", 36, NULL, 0);

	font12_l = LoadFontEx("resources/aller_light.ttf", 12, NULL, 0);
	font24_l = LoadFontEx("resources/aller_light.ttf", 24, NULL, 0);
	font36_l = LoadFontEx("resources/aller_light.ttf", 36, NULL, 0);

	std::vector<std::string> maps_getting_added;
	song_select_top_bar = LoadTextureCompat("resources/mode-osu-small.png");
	background = LoadTextureCompat((db::fs_path / "resources" / "default_bg.jpg").string().c_str());

	music = LoadMusicStream("resources/mus_menu.ogg");
	
	screen_width = (float)GetScreenWidth();
	screen_height = (float)GetScreenHeight();

	SetTargetFPS(1000);
	UpdateMusicStream(music);
	PlayMusicStream(music);

	while (!WindowShouldClose())
	{
		UpdateMusicStream(music);
		BeginDrawing();
		ClearBackground(DARKGRAY);
		if (IsKeyPressed(KEY_F5)) {
			game_state = MAIN_MENU;
			db::reconstruct_db();
		}
		switch (game_state) {
			case MAIN_MENU:
				DrawTextEx(font36, "Welcome to cppsu!", { 32, 32 }, 36, 0, WHITE);
				DrawTextEx(font24, "Press M to switch to the song select screen!", { 32, screen_height - 64 }, 24, 0, WHITE);
				DrawTextEx(font24, "Press N to import maps!", { 32, screen_height - 32 }, 24, 0, WHITE);
				if (IsKeyPressed(KEY_M)) {
					song_select::init();
				}
				if (IsKeyPressed(KEY_N)) {
					if (db::add_to_db(maps_getting_added)) {
					}
				}
			break;
			case IMPORTING:
				DrawRectangleGradientH(0, 0, screen_width, screen_height, YELLOW, ORANGE);
				DrawTextEx(font36, "Importing maps...", { 32, 32 }, 72, 0, BLACK);
				
				for (int i = 0; i < maps_getting_added.size(); i++) {

					DrawTextEx(font24, maps_getting_added[i].c_str(), {32, (float)128 + i * 24}, 24, 0, BLACK);
					i++;
				}
				if(importing_map == false) {
					maps_getting_added.clear();
					song_select::init();
					game_state = SONG_SELECT;
					break;
				}
			break;
			case SONG_SELECT:
				if (IsKeyPressed(KEY_B)) {
					game_state = MAIN_MENU;
				}
				if (IsKeyPressed(KEY_N)) {
					
					if (db::add_to_db(maps_getting_added)) {
					}
				}

				song_select::update();
				song_select::draw();
			break;
			case INGAME:
				
			break;
		}
		EndDrawing();
	}
	CloseWindow();
	return 0;
}
