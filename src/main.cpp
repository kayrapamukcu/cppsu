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

	font12 = LoadFontEx("resources/aller.ttf", 12, NULL, 0);
	font24 = LoadFontEx("resources/aller.ttf", 24, NULL, 0);
	font36 = LoadFontEx("resources/aller.ttf", 36, NULL, 0);

	font12_b = LoadFontEx("resources/aller_bold.ttf", 12, NULL, 0);
	font24_b = LoadFontEx("resources/aller_bold.ttf", 24, NULL, 0);
	font36_b = LoadFontEx("resources/aller_bold.ttf", 36, NULL, 0);

	font12_l = LoadFontEx("resources/aller_light.ttf", 12, NULL, 0);
	font24_l = LoadFontEx("resources/aller_light.ttf", 24, NULL, 0);
	font36_l = LoadFontEx("resources/aller_light.ttf", 36, NULL, 0);

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
		switch (game_state) {
			case MAIN_MENU:
				DrawTextEx(font36, "Welcome to cppsu!", { 32, 32 }, 36, 0, WHITE);
				DrawTextEx(font24, "Press M to switch to the song select screen!", { 32, screen_height - 64 }, 24, 0, WHITE);
				if (IsKeyPressed(KEY_M)) {
					song_select::init();
					game_state = SONG_SELECT;
				}
			break;
			case SONG_SELECT:	
				song_select::update();
				song_select::draw();
			break;
		}
		EndDrawing();
	}
	CloseWindow();
	return 0;
}
