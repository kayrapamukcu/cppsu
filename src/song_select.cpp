#include "song_select.hpp"
#include "raylib.h"
#include <iostream>
#include "globals.hpp"
#include <format>
#include <rlgl.h>
#include "ingame.hpp"
#include "result_screen.hpp"

float song_select::entry_row_height = 86.0f;
double song_select::current_position = 0.5;
float song_select::scroll_speed = 0.0f;
int song_select::visible_entries = 11;
int song_select::map_list_size = 0;
int song_select::selected_mapset = -999;
int song_select::selected_map_list_index = 0;
file_struct song_select::selected_map = file_struct();
int song_select::y_offset = 0;
int song_select::max_base = 0;
std::filesystem::path song_select::loaded_bg_path;
std::filesystem::path song_select::loaded_audio_path;
song_select::S_SUBMENU song_select::submenu = S_SUBMENU::NONE;

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
		SetMusicVolume(music, settings_volume_music / 100.0f);
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
		SetMusicVolume(music, settings_volume_music / 100.0f);
		SeekMusicStream(music, selected_map.preview_time / 1000.0f);
	}

	loaded_bg_path = bg_path;

	// load all scores
	score_list.clear();

	auto path = db::fs_path / "maps" / std::to_string(selected_map.beatmap_set_id) / std::to_string(selected_map.beatmap_id);
	if (std::filesystem::is_directory(path)) {
		auto filenames = db::get_files(path, ".score");
		if(filenames.size() > 0)
			load_score_list(path, filenames);
	}
}

void song_select::enter_game(file_struct map) {
	scroll_speed = 0.0f;
	StopMusicStream(music);
	game_state = INGAME;
	g_ingame = new ingame(map, selected_mods, score_multiplier, false, "");
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

	buttons_submenu_beatmap.clear();
	buttons_submenu_mods.clear();
	buttons_submenu_none.clear();

	auto sh = screen_height_ratio;
	buttons_submenu_none.push_back({
		SPRITE::Back,
		{0, 700 * sh}, {sh, sh},
		*callback_change_menu, 0, nullptr, "", WHITE
		});
	buttons_submenu_none.push_back({
		SPRITE::ButtonMods,
		{284 * sh, 684 * sh}, {sh, sh},
		*callback_change_menu, 2, nullptr, "", WHITE
		});
	buttons_submenu_none.push_back({
		SPRITE::ButtonRandom,
		{361 * sh, 684 * sh}, {sh, sh},
		*callback_choose_random_map, 0, nullptr, "", WHITE
		});
	buttons_submenu_none.push_back({
		SPRITE::ButtonBeatmapOptions,
		{438 * sh, 684 * sh}, {sh, sh},
		*callback_change_menu, 3, nullptr, "", WHITE
		});

	buttons_submenu_mods.push_back({
		SPRITE::ModEZ,
		{352 * sh, 229 * sh}, {sh, sh},
		*callback_change_mods, (int)MODS::EZ, *callback_draw_mods, "", WHITE
		});
	buttons_submenu_mods.push_back({
		SPRITE::ModNF,
		{458 * sh, 229 * sh}, {sh, sh},
		*callback_change_mods, (int)MODS::NF, *callback_draw_mods, "", WHITE
		});
	buttons_submenu_mods.push_back({
		SPRITE::ModHT,
		{564 * sh, 229 * sh}, {sh, sh},
		*callback_change_mods, (int)MODS::HT, *callback_draw_mods, "", WHITE
		});
	buttons_submenu_mods.push_back({
		SPRITE::ModHR,
		{352 * sh, 325 * sh}, {sh, sh},
		*callback_change_mods, (int)MODS::HR, *callback_draw_mods, "", WHITE
		});
	SPRITE spr_option = SPRITE::ModSD;
	MODS mod_option = MODS::SD;

	if (selected_mods[(int)MODS::PF]) {
		spr_option = SPRITE::ModPF;
		mod_option = MODS::PF;
	}
	buttons_submenu_mods.push_back({
		spr_option,
		{458 * sh, 325 * sh}, {sh, sh},
		*callback_change_mods, (int)mod_option, *callback_draw_mods, "", WHITE
		});
	spr_option = SPRITE::ModDT;
	mod_option = MODS::DT;

	if (selected_mods[(int)MODS::NC]) {
		spr_option = SPRITE::ModNC;
		mod_option = MODS::NC;
	}

	buttons_submenu_mods.push_back({
		spr_option,
		{564 * sh, 325 * sh}, {sh, sh},
		*callback_change_mods, (int)mod_option, *callback_draw_mods, "", WHITE
		});
	buttons_submenu_mods.push_back({
		SPRITE::ModHD,
		{670 * sh, 325 * sh}, {sh, sh},
		*callback_change_mods, (int)MODS::HD, *callback_draw_mods, "", WHITE
		});
	buttons_submenu_mods.push_back({
		SPRITE::ModFL,
		{776 * sh, 325 * sh}, {sh, sh},
		*callback_change_mods, (int)MODS::FL, *callback_draw_mods, "", WHITE
		});

	buttons_submenu_mods.push_back({
		SPRITE::ModRX,
		{352 * sh, 421 * sh}, {sh, sh},
		*callback_change_mods, (int)MODS::RX, *callback_draw_mods, "", WHITE
		});
	buttons_submenu_mods.push_back({
		SPRITE::ModAP,
		{458 * sh, 421 * sh}, {sh, sh},
		*callback_change_mods, (int)MODS::AP, *callback_draw_mods, "", WHITE
		});
	buttons_submenu_mods.push_back({
		SPRITE::ModSO,
		{564 * sh, 421 * sh}, {sh, sh},
		*callback_change_mods, (int)MODS::SO, *callback_draw_mods, "", WHITE
		});
	buttons_submenu_mods.push_back({
		SPRITE::ModAT,
		{670 * sh, 421 * sh}, {sh, sh},
		*callback_change_mods, (int)MODS::AT, *callback_draw_mods, "", WHITE
		});
	buttons_submenu_mods.push_back({
		SPRITE::ModRC,
		{776 * sh, 421 * sh}, {sh, sh},
		*callback_change_mods, (int)MODS::RC, *callback_draw_mods, "", WHITE
		});
	buttons_submenu_mods.push_back({
		SPRITE::ModDA,
		{882 * sh, 421 * sh}, {sh, sh},
		*callback_change_mods, (int)MODS::DA, *callback_draw_mods, "", WHITE
		});

	buttons_submenu_mods.push_back({
		SPRITE::TOTAL_COUNT,
		{151 * sh, 552 * sh}, {723 * screen_width_ratio, 56 * sh},
		*callback_change_mods, (int)MODS::COUNT, nullptr, "1. Reset All Mods", { 225, 47, 0, 255 }
		});
	buttons_submenu_mods.push_back({
		SPRITE::TOTAL_COUNT,
		{151 * sh, 632 * sh}, {723 * screen_width_ratio, 56 * sh},
		*callback_change_menu, 1, nullptr, "2. Close", { 107, 107, 107, 255 }
		});

	if(IsMusicValid(music)) SetMusicPitch(music, 1.0f);
	game_state = SONG_SELECT;
	choose_beatmap(selected_map_list_index);
	submenu = S_SUBMENU::NONE;
}

void song_select::callback_choose_random_map(int) {
	choose_beatmap(GetRandomValue(0, map_list_size));
	current_position = selected_map_list_index - 4;
}
void song_select::callback_change_menu(int menu)
{
	switch (menu) {
	case 0:
		game_state = MAIN_MENU;
		break;
	case 1:
		submenu = S_SUBMENU::NONE;
		break;
	case 2:
		submenu = S_SUBMENU::MODS;
		break;
	case 3:
		submenu = S_SUBMENU::BEATMAP;
		break;
	}
}
void song_select::callback_change_mods(int i)
{
	score_multiplier = 1.0f;
	if (i == (int)MODS::COUNT) {
		for (auto& m : selected_mods) {
			m = false;
		}
		selected_mods_string.clear();
		return;
	}

	MODS mod = (MODS)i;

	switch (mod) {
	case MODS::DT:
		selected_mods[(int)MODS::HT] = false;
		if (selected_mods[i]) {
			selected_mods[i] = false;
			selected_mods[(int)MODS::NC] = true;
			// magic number time
			buttons_submenu_mods[5].sprite = SPRITE::ModNC;
			buttons_submenu_mods[5].id = (int)MODS::NC;
		} else selected_mods[i] = !selected_mods[i];
		break;
	case MODS::NC:
		selected_mods[(int)MODS::HT] = false;
		selected_mods[i] = !selected_mods[i];
		buttons_submenu_mods[5].sprite = SPRITE::ModDT;
		buttons_submenu_mods[5].id = (int)MODS::DT;
		break;
	case MODS::HR:
		selected_mods[i] = !selected_mods[i];
		selected_mods[(int)MODS::EZ] = false;
		break;
	case MODS::EZ:
		selected_mods[i] = !selected_mods[i];
		selected_mods[(int)MODS::HR] = false;
		break;
	case MODS::HT:
		selected_mods[i] = !selected_mods[i];
		selected_mods[(int)MODS::DT] = false;
		selected_mods[(int)MODS::NC] = false;
		buttons_submenu_mods[5].sprite = SPRITE::ModDT;
		buttons_submenu_mods[5].id = (int)MODS::DT;
		break;
	case MODS::SD:
		selected_mods[(int)MODS::NF] = false;
		if (selected_mods[i]) {
			selected_mods[i] = false;
			selected_mods[(int)MODS::PF] = true;
			// magic number time
			buttons_submenu_mods[4].sprite = SPRITE::ModPF;
			buttons_submenu_mods[4].id = (int)MODS::PF;
		}
		else selected_mods[i] = !selected_mods[i];
		break;
	case MODS::PF:
		selected_mods[(int)MODS::NF] = false;
		selected_mods[i] = !selected_mods[i];
		buttons_submenu_mods[4].sprite = SPRITE::ModSD;
		buttons_submenu_mods[4].id = (int)MODS::SD;
		break;
	case MODS::NF:
		selected_mods[i] = !selected_mods[i];
		selected_mods[(int)MODS::SD] = false;
		selected_mods[(int)MODS::PF] = false;
		buttons_submenu_mods[4].sprite = SPRITE::ModSD;
		buttons_submenu_mods[4].id = (int)MODS::SD;
		break;
	default:
		selected_mods[i] = !selected_mods[i];
		break;
	}

	
	// calculate score mult. 
	for (int i = 0; i < selected_mods.size(); i++) {
		if (selected_mods[i]) {
			score_multiplier *= std::get<2>(mod_info[i]);
		}
	}

	// create mod string
	selected_mods_string.clear();
	for (int i = 0; i < selected_mods.size(); i++) {
		
		if (selected_mods[i]) {
			if (selected_mods_string != "") {
				selected_mods_string += ",";
			}
			selected_mods_string += std::get<3>(mod_info[i]);
		}
	}
}
void song_select::callback_draw_mods(button_callback& b)
{
	if (selected_mods[b.id]) {
		Rectangle src = tex[(int)b.sprite];
		float w = src.width * b.scale.x * 1.12f;
		float h = src.height * b.scale.y * 1.12f;

		DrawTexturePro(atlas, src, { b.pos.x + w * 0.5f, b.pos.y + h * 0.5f, w, h }, { w * 0.56f, h * 0.56f }, 10.0f, b.color);
		DrawRectanglePro({ b.pos.x + w * 0.5f, b.pos.y + h * 0.5f, w, h }, { w * 0.56f, h * 0.56f }, 10.0f, { 255, 255, 255, 100 });
	}
	else
		DrawTexturePro(atlas, tex[(int)b.sprite], { b.pos.x, b.pos.y, tex[(int)b.sprite].width * b.scale.x, tex[(int)b.sprite].height * b.scale.y }, { 0.0f, 0.0f }, 0.0f, b.color);
}

void song_select::load_score_list(const std::filesystem::path path, const std::vector<std::string> scores)
{
	for (auto& s : scores) {
		results_struct data;
		std::ifstream fin(path / s, std::ios::binary);
		fin.read(reinterpret_cast<char*>(&data), sizeof(results_struct));

		score_entry score;
		std::string mod_text_full;
		score.score = data.score;
		score.acc_text = format_floats(data.accuracy) + "%";
		score.mod_array = data.mod_array;
		score.filename = s;
		for (int i = 0; i < score.mod_array.size(); i++) {
			if(score.mod_array[i])
				if (score.mod_text.empty()) {
					score.mod_text += std::get<1>(mod_info[i]);
					mod_text_full = std::get<3>(mod_info[i]);
				}
				else {
					score.mod_text += "," + std::string(std::get<1>(mod_info[i]));
					mod_text_full += "," + std::string(std::get<3>(mod_info[i]));
				}	
		}
		score.score_text = "Score: " + format_int(data.score) + " (" + std::to_string(data.max_combo) + "x)";

		score.text_widths[0] = MeasureTextEx(aller_r, score.acc_text.c_str(), 16.f * screen_height_ratio, 0).x;
		score.text_widths[1] = MeasureTextEx(aller_r, score.mod_text.c_str(), 16.f * screen_height_ratio, 0).x;
		

		std::time_t t = std::chrono::system_clock::to_time_t(data.time);
		std::tm local_tm{};

		#if defined(_WIN32)
				localtime_s(&local_tm, &t); // Windows
		#else
				localtime_r(&t, &local_tm); // MacOS / Linux
		#endif

		std::strftime(score.time_buf.data(), sizeof(score.time_buf), "%d.%m.%Y %H:%M:%S", &local_tm);

		score.hover_text = "Achieved on " + std::string(score.time_buf.data()) + "\n300: " + std::to_string(data.hit300s) + " 100: " + std::to_string(data.hit100s) + " 50: " + std::to_string(data.hit50s) + " Miss: " + std::to_string(data.misses) + "\nAccuracy:" + format_floats(data.accuracy) + "%\nMods: " + (mod_text_full.empty() ? "None" : mod_text_full);

		score.hover_text_size = MeasureTextEx(aller_r, score.hover_text.c_str(), 18.f * screen_height_ratio, 0);

		
		score.name = data.player_name;
		score.rank = data.rank;

		

		score_list.push_back(score);

		fin.close();
	}

	std::sort(score_list.begin(), score_list.end(), [](const score_entry& a, const score_entry& b) {
		return a.score > b.score;
	});
}

void song_select::update() {
	
	
	switch (submenu) {
	case S_SUBMENU::NONE: {

		for (auto& b : buttons_submenu_none) {
			update_button(b);
		}
		scroll_speed += IsKeyPressed(KEY_DOWN) * 3.0f + IsKeyPressed(KEY_UP) * -3.0f;
		scroll_speed -= GetMouseWheelMove();
		scroll_speed *= std::max(0.0f, 1.0f - frame_time);

		current_position += scroll_speed * 0.05 * (frame_time * 120.0f);
		constexpr double extra_space = 3.0f;
		if (map_list_size > 6)
			current_position = std::clamp(current_position, -extra_space, (double)max_base + extra_space);
		else
			current_position = std::clamp(current_position, 0.0, (double)max_base);

		double pos_idx = std::clamp(current_position, 0.0, (double)max_base);

		int base = (int)std::floor(current_position);

		float frac = (float)current_position - (float)base;
		float y_origin, x_origin;

		// check for clicks
		if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
			// entries
			if (cursor.x > screen_height * 0.1 && cursor.y < screen_height * 0.9) {
				for (int i = 0; i < visible_entries; ++i) {
					y_origin = screen_height_ratio * (y_offset + 32.0f + i * entry_row_height - frac * entry_row_height);
					x_origin = screen_width_ratio * (512.0f + abs(screen_height / 2 - y_origin) * 0.1f);
					if (cursor.y >= y_origin && cursor.y <= y_origin + entry_row_height * screen_height_ratio && cursor.x > x_origin) {
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
			// scores

			for (int i = 0; i < score_list.size(); i++) {
				if (CheckCollisionPointRec(cursor, { 5.f * screen_height_ratio, (153.f + 53.f * i) * screen_height_ratio, 384.f * screen_height_ratio, 48.f * screen_height_ratio })) {
					auto path = db::fs_path / "maps" / std::to_string(selected_map.beatmap_set_id) / std::to_string(selected_map.beatmap_id) / score_list[i].filename;

					std::ifstream file(path, std::ios::binary);

					results_struct res;
					const auto n = 512 - sizeof(res);
					char buffer[n];

					file.read((char*)(&res), sizeof(res));
					file.read(buffer, n);

					g_result_screen = new result_screen(res, selected_map);
					game_state = RESULT_SCREEN;
				}
			}
		}
			
		if (IsKeyPressed(KEY_ENTER)) enter_game(selected_map);
		if (IsKeyPressed(KEY_F1)) submenu = S_SUBMENU::MODS;
		if (IsKeyPressed(KEY_F2)) {
			callback_choose_random_map(0);
		}
		if (IsKeyPressed(KEY_F3)) submenu = S_SUBMENU::BEATMAP;
		if (IsKeyPressed(KEY_ESCAPE)) game_state = MAIN_MENU;
		break;
	}
	case S_SUBMENU::MODS:
		if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_F1) || IsKeyPressed(KEY_TWO)) submenu = S_SUBMENU::NONE;
		if (IsKeyPressed(KEY_ONE)) callback_change_mods((int)MODS::COUNT);
		for (auto& b : buttons_submenu_mods) {
			update_button(b);
		}
		break;
	case S_SUBMENU::BEATMAP:
		if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_F3)) submenu = S_SUBMENU::NONE;
		break;
	}

}

void song_select::draw() {
	if (map_list.empty()) return;
	DrawTextureCompatPro(background, { 0,0, screen_width, screen_height }, WHITE);
	DrawRectangleRec({ 0, 0, screen_width, screen_height }, { 0, 0, 0, 100 });

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

	Color map_stats_c = WHITE;
	float display_cs = selected_map.cs;
	float display_ar = selected_map.ar;
	float display_od = selected_map.od;
	float display_hp = selected_map.hp;

	if (selected_mods[(int)MODS::HR]) {
		map_stats_c = { 246, 154, 161, 255 };
		display_cs *= 1.3f;
		display_ar *= 1.4f;
		display_od *= 1.4f;
		display_hp *= 1.4f;
		if (display_cs > 10.0f) display_cs = 10.0f;
		if (display_ar > 10.0f) display_ar = 10.0f;
		if (display_od > 10.0f) display_od = 10.0f;
		if (display_hp > 10.0f) display_hp = 10.0f;
	}
	else if (selected_mods[(int)MODS::EZ]) {
		map_stats_c = { 173, 216, 230, 255 };
		display_cs *= 0.5f;
		display_ar *= 0.5f;
		display_od *= 0.5f;
		display_hp *= 0.5f;
	}

	std::string stats_3 = std::format(
		"CS:{}  AR:{}  OD:{}  HP:{}  Stars:{}",
		format_floats(display_cs),
		format_floats(display_ar),
		format_floats(display_od),
		format_floats(display_hp),
		format_floats(selected_map.star_rating)
	);
	DrawTextExScaled(aller_r, stats_3.c_str(), { 4,112 }, 18, 0, map_stats_c);

	// draw loaded scores

	Rectangle draw_on_top;
	std::string hover_text;

	for (int i = 0; i < score_list.size(); i++) {
		DrawRectangleRec({ 5.f * screen_height_ratio, (153.f + 53.f * i) * screen_height_ratio, 384.f * screen_height_ratio, 48.f * screen_height_ratio }, { 0, 0, 0, 80 });
		score_entry& s = score_list[i];
		Rectangle rank_to_draw = tex[(int)SPRITE::RankDSmall];
		switch (s.rank) {
		case RANK_XH:
			rank_to_draw = tex[(int)SPRITE::RankXHSmall];
			break;
		case RANK_X:
			rank_to_draw = tex[(int)SPRITE::RankXSmall];
			break;
		case RANK_SH:
			rank_to_draw = tex[(int)SPRITE::RankSHSmall];
			break;
		case RANK_S:
			rank_to_draw = tex[(int)SPRITE::RankSSmall];
			break;
		case RANK_A:
			rank_to_draw = tex[(int)SPRITE::RankASmall];
			break;
		case RANK_B:
			rank_to_draw = tex[(int)SPRITE::RankBSmall];
			break;
		case RANK_C:
			rank_to_draw = tex[(int)SPRITE::RankCSmall];
			break;
		case RANK_D:
			rank_to_draw = tex[(int)SPRITE::RankDSmall];
			break;
		}

		DrawTexturePro(atlas, rank_to_draw, { 60.f * screen_height_ratio, (157.f + 53.f * i) * screen_height_ratio , rank_to_draw.width * screen_height_ratio, rank_to_draw.height * screen_height_ratio }, { 0.0f, 0.0f }, 0.0f, WHITE);
		DrawTextEx(aller_b, s.name.data(), { 100.f * screen_height_ratio, (157.f + 53.f * i) * screen_height_ratio }, 24.f * screen_height_ratio, 0.0f, WHITE);
		DrawTextEx(aller_r, s.score_text.data(), { 100.f * screen_height_ratio, (180.f + 53.f * i) * screen_height_ratio }, 18.f * screen_height_ratio, 0.0f, WHITE);

		DrawTextEx(aller_r, s.acc_text.data(), { 378.f * screen_height_ratio - s.text_widths[0], (171.f + 53.f * i) * screen_height_ratio}, 16.f * screen_height_ratio, 0.0f, WHITE);
		DrawTextEx(aller_r, s.mod_text.data(), { 378.f * screen_height_ratio - s.text_widths[1], (157.f + 53.f * i) * screen_height_ratio}, 16.f * screen_height_ratio, 0.0f, WHITE);

		if (CheckCollisionPointRec(cursor, { 5.f * screen_height_ratio, (153.f + 53.f * i) * screen_height_ratio, 384.f * screen_height_ratio, 48.f * screen_height_ratio })) {
			draw_on_top = { cursor.x, cursor.y, s.hover_text_size.x, s.hover_text_size.y };
			hover_text = s.hover_text;
		}
	}
	if (!hover_text.empty()) {
		DrawRectangleRec(draw_on_top, { 0, 0, 0, 220 });
		DrawTextEx(aller_r, hover_text.c_str(), { cursor.x, cursor.y }, 18.f * screen_height_ratio, 0.0f, WHITE);
	}

	DrawRectangleRec({ 0, 684 * screen_height_ratio, screen_width, screen_height * 0.14f }, BLACK);

	for (auto& b : buttons_submenu_none) {
		draw_button(b);
	}

	DrawTextEx(aller_l, selected_mods_string.c_str(), { 106 * screen_height_ratio, 642 * screen_height_ratio }, 48.0f * screen_height_ratio, 0, { 200, 200, 200, 180 });

	switch (submenu) {
	case S_SUBMENU::MODS: {
		DrawRectangleRec({ 0, 0, screen_width, screen_height }, { 0, 0, 0, 200 });

		std::snprintf(score_str, sizeof(score_str), "%.2f", score_multiplier);
		Color score_color = WHITE;
		if (score_multiplier > 1.0f) {
			score_color = GREEN;
		}
		else if (score_multiplier < 1.0f) {
			score_color = RED;
		}
		std::string score_text = "Score Multiplier: " + std::string(score_str) + "x";
		float score_width = MeasureTextEx(aller_r, score_text.c_str(), 48.0f * screen_height_ratio, 0).x;

		DrawTextEx(aller_l, score_text.c_str(), { screen_width / 2.f - score_width / 2.f, 153.f * screen_height_ratio }, 48.0f * screen_height_ratio, 0.0f, score_color);
		DrawTextEx(aller_r, "Difficulty Reduction", { 40.f * screen_height_ratio, 245.f * screen_height_ratio }, 36.0f * screen_height_ratio, 0, GREEN);
		DrawTextEx(aller_r, "Difficulty Increase", { 40.f * screen_height_ratio, 341.f * screen_height_ratio }, 36.0f * screen_height_ratio, 0, RED);
		DrawTextEx(aller_r, "Special", { 40.f * screen_height_ratio, 437.f * screen_height_ratio }, 36.0f * screen_height_ratio, 0, WHITE);

		for (auto& b : buttons_submenu_mods) {
			if (b.draw != nullptr) {
				b.draw(b);
			}
			else {
				SetTextureFilter(atlas, TEXTURE_FILTER_POINT);
				draw_button(b);
				SetTextureFilter(atlas, TEXTURE_FILTER_BILINEAR);
			}
				
		}
		break;
	}
	case S_SUBMENU::BEATMAP:
		DrawRectangleRec({ 0, 0, screen_width, screen_height }, { 0, 0, 0, 200 });
		break;
	}

	
}

