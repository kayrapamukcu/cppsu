#include "raylib.h"
#include "rlgl.h"
#include "json.hpp"

#include <vector>
#include <string>
#include <filesystem>
#include <iostream>
#include <format>
#include <chrono>
#include <thread>

#include "db.hpp"
#include "globals.hpp"
#include "song_select.hpp"
#include "ingame.hpp"
#include "result_screen.hpp"
#include "settings.hpp"

#include <GLFW/glfw3.h>

using json = nlohmann::json;

int main()
{	
	std::ios::sync_with_stdio(false);
	// INIT SEQUENCE
	db::init();	
	InitWindow((int)screen_width, (int)screen_height, "cppsu!");
	InitAudioDevice();

	

	// build sound effects

	std::filesystem::path sfx_path = db::fs_path / "resources" / "sounds";


	auto files_vect = db::get_files(sfx_path, std::vector<std::string>{".wav", ".mp3"});

	for(const auto& f : files_vect) {
		Sound sfx = LoadSound((sfx_path / f).string().c_str());
		sound_effects[f] = sfx;
		std::cout << std::format("Loaded SFX: {}\n", f);
	}

	SetExitKey(KEY_NULL);

	aller_r = LoadFontEx("resources/aller.ttf", 72, NULL, 0);
	aller_l = LoadFontEx("resources/aller_light.ttf", 72, NULL, 0);
	aller_b = LoadFontEx("resources/aller_bold.ttf", 72, NULL, 0);

	SetTextureFilter(aller_r.texture, TEXTURE_FILTER_BILINEAR);
	SetTextureFilter(aller_l.texture, TEXTURE_FILTER_BILINEAR);
	SetTextureFilter(aller_b.texture, TEXTURE_FILTER_BILINEAR);

	

	// Load Atlas

	for (int i = 0; i < (int)SPRITE::TOTAL_COUNT; ++i) {
		tex[i] = { 0,0,0,0 };
	}

	std::ifstream f("resources/atlas_data.json");
	json j = json::parse(f);

	atlas = LoadTexture("resources/atlas.png");
	SetTextureFilter(atlas, TEXTURE_FILTER_BILINEAR);
	for (auto& [filename, frame] : j["frames"].items()) {

		Rectangle rect = { (float)frame["frame"]["x"], (float)frame["frame"]["y"], (float)frame["frame"]["w"], (float)frame["frame"]["h"] };
		for (auto& m : atlas_map) {
			if (filename == m.name) {
				tex[(int)m.id] = rect;
				break;
			}
		}
	}

	HideCursor();

	std::vector<std::string> maps_getting_added;
	
	song_select_top_bar = LoadTexture("resources/textures/mode-osu-small.png");
	background = LoadTextureCompat((db::fs_path / "resources" / "textures" / "default_bg.jpg").string().c_str());

	auto welcome_sfx = LoadSound("resources/welcome.mp3");

	music = LoadMusicStreamFromRam("resources/mus_menu.ogg");
	
	screen_width = (float)GetScreenWidth();
	screen_height = (float)GetScreenHeight();

	// SetTargetFPS(360);

	settings::init();
	SetSoundPitch(welcome_sfx, 0.6f);
	PlaySound(welcome_sfx);

	// build white 1x1 image

	auto renderTexture = LoadRenderTexture(1, 1);
	BeginTextureMode(renderTexture);
	ClearBackground(WHITE);
	EndTextureMode();
	
	// 3.56s welcome sound

	auto start_time = GetTime();
	float passed_time_ratio;

	Color fps_orange = { 255, 149, 24, 255 };
	Color fps_yellow = { 255, 204, 34, 255 };
	Color fps_green = { 172, 220, 25, 255 };

	game_state = MAIN_MENU;

	while (!WindowShouldClose())
	{
		if (IsKeyPressed(KEY_F2)) {
			
			EnableCursor();
			SetMousePosition(cursor.x, cursor.y);
			HideCursor();
			settings_raw_input = false;
		}
		if (IsKeyPressed(KEY_F3)) {
			
			DisableCursor();
			settings_raw_input = true;
		}
		UpdateMusicStream(music);
		if (IsKeyPressed(KEY_F4)) {
			ToggleFullscreen();
		}
		if (IsKeyPressed(KEY_F5)) {
			game_state = MAIN_MENU;
			db::reconstruct_db();
		}

		if (IsKeyPressed(KEY_F6)) {
			settings::update_screen_resolution(screen_width + 16 * 5, screen_height + 9 * 5);
		}
		if (IsKeyPressed(KEY_F7)) {
			settings::update_screen_resolution(screen_width - 16 * 5, screen_height - 9 * 5);
		}

		BeginDrawing();
		
		// TODO : add buttons instead of keybinds

		switch (game_state) {
			case INIT:
				ClearBackground(BLACK);
				passed_time_ratio = float(GetTime() - start_time) / 3.56f;

				DrawTexturePro(
					renderTexture.texture,
					{ 0, 0, 1.0f, 1.0f },
					{ 0, 0, screen_width, screen_height },
					{ 0,0 },
					0.0f,
					{ 255, 255, 255, (unsigned char)(255 * passed_time_ratio) }
				);

				if (start_time + 3.56 < GetTime()) {
					game_state = MAIN_MENU;
					PlayMusicStream(music);
					HideCursor();
				}
					
				DrawTextExScaled(aller_r, "Loading...", { screen_width / 2 - 100, screen_height / 2 - 24 }, 48 * screen_scale, 0, WHITE);
				goto end_draw;
			break;
			case MAIN_MENU:
				ClearBackground(DARKGRAY);
				
				DrawTextExScaled(aller_r, "Welcome to cppsu!", { 32, 32 }, 36*screen_scale, 0, WHITE);
				DrawTextExScaled(aller_r, "Press S to open the settings!", { 32, screen_height - 96 }, 24 * screen_scale, 0, WHITE);
				DrawTextExScaled(aller_r, "Press M to switch to the song select screen!", { 32, screen_height - 64 }, 24*screen_scale, 0, WHITE);
				DrawTextExScaled(aller_r, "Press N to import maps!", { 32, screen_height - 32 }, 24*screen_scale, 0, WHITE);
				if (IsKeyPressed(KEY_M)) {
					song_select::init(false);
				}
				if (IsKeyPressed(KEY_N)) {
					db::get_last_assigned_id();
					if (db::add_to_db(maps_getting_added)) {
					}
				}
				if (IsKeyPressed(KEY_S)) {
					game_state = SETTINGS;
				}
			break;
			case IMPORTING:
				DrawRectangleGradientH(0, 0, (int)screen_width, (int)screen_height, YELLOW, ORANGE);
				DrawTextExScaled(aller_r, "Importing maps...", { 32, 32 }, 108, 0, BLACK);
				for (size_t i = 0; i < maps_getting_added.size(); i++) {

					DrawTextExScaled(aller_r, maps_getting_added[i].c_str(), {32, (float)128 + i * 24}, 24, 0, BLACK);
					i++;
				}
				if(importing_map == false) {
					std::this_thread::sleep_for(std::chrono::milliseconds(300));
					maps_getting_added.clear();
					song_select::init(false);
					game_state = SONG_SELECT;
					break;
				}
			break;
			case SETTINGS: 
				settings::update();
			break;
			case SONG_SELECT:
				// move all this shit to update()
				// TODO : add right click
				if (IsKeyPressed(KEY_B)) {
					game_state = MAIN_MENU;
				}
				if (IsKeyPressed(KEY_N)) {
					
					if (db::add_to_db(maps_getting_added)) {
					}
				}
				if (IsKeyPressed(KEY_F5)) {
					game_state = MAIN_MENU;
					db::reconstruct_db();
					song_select::init(false);
				}

				if (IsKeyPressed(KEY_K)) { // delete entire set
					game_state = MAIN_MENU;
					db::remove_from_db(0, song_select::selected_map.beatmap_id, song_select::selected_map.beatmap_set_id);
					song_select::init(false);
				}
				if (IsKeyPressed(KEY_L)) { // delete single map
					game_state = MAIN_MENU;
					db::remove_from_db(1, song_select::selected_map.beatmap_id, song_select::selected_map.beatmap_set_id);
					song_select::init(false);
				}

				song_select::update();
				song_select::draw();
			break; 
			case INGAME:
				g_ingame->update();
				if(g_ingame) g_ingame->draw();
				if (IsKeyPressed(KEY_B)) {
					song_select::init(true);
				}
			break;
			case RESULT_SCREEN: 
				g_result_screen->update();
				if(g_result_screen) g_result_screen->draw();
			break;
		}

		for (size_t i = 0; i < notices.size(); i++) {
			auto& n = notices[i];
			if (n.time_left < 0) {
				notices.erase(notices.begin() + i);
				i--;
				continue;
			}
			
			std::string lines;
			int current_line_length = 0;
			for (auto& n : n.text) {
				lines += n;
				current_line_length++;
				if(current_line_length > 20 && n == ' ') {
					lines += '\n';
					current_line_length = 0;
				}
			}
			DrawRectangle((int)(766.0f * screen_width_ratio), (int)(550.0f * screen_height_ratio), (int)(204.0f * screen_width_ratio), (int)(154.0f * screen_height_ratio), PURPLE);
			DrawRectangle((int)(768.0f * screen_width_ratio), (int)(552.0f * screen_height_ratio), (int)(200.0f * screen_width_ratio), (int)(150.0f * screen_height_ratio), BLACK);
			DrawTextExScaled(aller_r, lines.c_str(), { 772, 552 }, 18, 0, WHITE);
			n.time_left -= GetFrameTime();
		}

		// draw cursor and snap back to borders if raw input is on
		{
			
			
			if (settings_raw_input) {

				Vector2 d = GetMouseDelta();
				cursor.x += d.x * settings_mouse_sens;
				cursor.y += d.y * settings_mouse_sens;

				if (cursor.x < 0) {
					cursor.x = 0;
				}
				else if (cursor.x > screen_width) {
					cursor.x = screen_width;
				}

				if (cursor.y < 0) {
					cursor.y = 0;
				}
				else if (cursor.y > screen_height) {
					cursor.y = screen_height;
				}
			}
			else {
				cursor = GetMousePosition();
			}
			auto& c = tex[(int)SPRITE::Cursor];
			DrawTexturePro(atlas, c, { cursor.x - (c.width * settings_mouse_scale / 2) * screen_scale, cursor.y - (c.height * settings_mouse_scale / 2) * screen_scale, c.width * settings_mouse_scale * screen_scale, c.height * settings_mouse_scale * screen_scale }, { 0,0 }, 0.0f, WHITE);
		}



		// draw fps / ms info

		if (settings_render_fps_ms) {
			int fps_target = screen_refresh_rate * 2;
			int fps = GetFPS();
			Color color_fps;

			if (fps_target <= fps) color_fps = fps_green;
			else if (fps_target <= fps * 1.05f) color_fps = fps_yellow;
			else color_fps = fps_orange;
			DrawRectangleV({ screen_width - (76.0f * screen_height_ratio), screen_height - (56.0f * screen_height_ratio) }, { 68.0f * screen_height_ratio , 20.0f * screen_height_ratio }, color_fps);

			float frametime = GetFrameTime() * 1000.0f;
			Color color_frametime;
			if (frametime <= 6.0f) color_frametime = fps_green;
			else if (frametime <= 12.0f) color_frametime = fps_yellow;
			else color_frametime = fps_orange;

			DrawRectangleV({ screen_width - (76.0f * screen_height_ratio), screen_height - (24.0f * screen_height_ratio) }, { 68.0f * screen_height_ratio , 20.0f * screen_height_ratio }, color_frametime);

			std::string fps_text = std::to_string(fps) + "fps";
			float fps_offset = MeasureTextEx(aller_r, fps_text.c_str(), 18 * screen_height_ratio, 0).x;
			DrawTextEx(aller_r, fps_text.c_str(), { screen_width - (42.0f * screen_height_ratio) - fps_offset / 2, screen_height - (56.0f * screen_height_ratio) }, 18 * screen_height_ratio, 0, BLACK);

			std::string frametime_text = (format_floats(frametime) + "ms").c_str();
			float frametime_offset = MeasureTextEx(aller_r, frametime_text.c_str(), 18 * screen_height_ratio, 0).x;
			DrawTextEx(aller_r, frametime_text.c_str(), { screen_width - (42.0f * screen_height_ratio) - frametime_offset / 2, screen_height - (24.0f * screen_height_ratio) }, 18 * screen_height_ratio, 0, BLACK);
		}

		end_draw:
		EndDrawing();
	}
	CloseWindow();
	return 0;
}
