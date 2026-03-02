#include "result_screen.hpp"
#include "song_select.hpp"
#include <format>

result_screen::result_screen(results_struct results)
{
	this->results = results;
	beatmap_header = std::string(results.artist.data()) + " - " + std::string(results.title.data()) + " [" + std::string(results.difficulty.data()) + "]";
	beatmap_header_2 = "Beatmap by " + std::string(results.creator.data());
	save_score();
	StopMusicStream(music);
	score_str = get_score_string(results.score);

	// can't use modern chrono functions because we're trying to support old operating systems

	std::time_t t = std::chrono::system_clock::to_time_t(results.time);
	std::tm local_tm{};

	#if defined(_WIN32)
		localtime_s(&local_tm, &t); // Windows
	#else
		localtime_r(&t, &local_tm); // MacOS / Linux
	#endif

	char time_buf[32];
	std::strftime(time_buf, sizeof(time_buf), "%d.%m.%Y %H:%M:%S", &local_tm);

	played_text = "Played by " + std::string(results.player_name.data()) + " on " + std::string(time_buf) + ".";

	channel_music = play_sound_effect("applause.mp3");
	switch (results.rank) {
	case RANK_XH:
		rank_to_draw = tex[(int)SPRITE::RankXH];
		break;
	case RANK_X:
		rank_to_draw = tex[(int)SPRITE::RankX];
		break;
	case RANK_SH:
		rank_to_draw = tex[(int)SPRITE::RankSH];
		break;
	case RANK_S:
		rank_to_draw = tex[(int)SPRITE::RankS];
		break;
	case RANK_A:
		rank_to_draw = tex[(int)SPRITE::RankA];
		break;
	case RANK_B:
		rank_to_draw = tex[(int)SPRITE::RankB];
		break;
	case RANK_C:
		rank_to_draw = tex[(int)SPRITE::RankC];
		break;
	case RANK_D:
		rank_to_draw = tex[(int)SPRITE::RankD];
		break;
	}

	std::string cv_ur = "";
	if (results.map_speed != 1.0f) 
		cv_ur = "\n(" + format_floats(results.unstable_rate[1] / results.map_speed) + " cv.UR)";

	ur_text = "Accuracy:\nError: " + format_floats(results.unstable_rate[0]) + "ms to " + format_floats(results.unstable_rate[2]) + "ms avg\nUnstable Rate: " + format_floats(results.unstable_rate[1]) + cv_ur;
}

void result_screen::draw() {
	
	DrawTextureCompatPro(background, { 0,0, screen_width, screen_height }, WHITE);
	
	float sh = screen_height_ratio;

	DrawRectangleGradientV(16 * sh, 120 * sh, 600 * sh, 64 * sh, Color{ 228, 206, 167, 255 }, Color{ 178, 157, 138, 255 });
	DrawTextEx(aller_b, score_str.c_str(), { 176 * sh, 120 * sh }, 72 * sh, 0, WHITE);
	
	DrawRectangleGradientV(16 * sh, 200 * sh, 600 * sh, 400 * sh, Color{ 53, 128, 219, 255 }, Color{ 35, 91, 165, 255 });
	DrawTextEx(aller_l, "Combo", { 18 * sh, 500 * sh }, 48 * sh, 0, WHITE);

	DrawTextEx(aller_b, (std::to_string(results.max_combo) + "x").c_str(), { 28.0f * sh, 530.0f * sh }, 60.0f * sh, 0, WHITE);

	DrawTextEx(aller_l, "Accuracy", { 350 * sh, 500 * sh }, 48 * sh, 0, WHITE);
	DrawTextEx(aller_b, (std::format("{:.2f}%", results.accuracy)).c_str(), { 360.0f * sh, 530.0f * sh }, 60.0f * sh, 0, WHITE);

	DrawTextEx(aller_b, (std::to_string(results.hit300s) + "x").c_str(), { 120.0f * sh, 256.0f * sh}, 48 * sh, 0, WHITE);
	DrawTextEx(aller_b, (std::to_string(results.hit100s) + "x").c_str(), { 120.0f * sh, 336.0f * sh}, 48 * sh, 0, WHITE);
	DrawTextEx(aller_b, (std::to_string(results.hit50s) + "x").c_str(), { 120.0f * sh, 416.0f * sh}, 48 * sh, 0, WHITE);

	DrawTextEx(aller_b, (std::to_string(results.geki) + "x").c_str(), { 450.0f * sh, 256.0f * sh }, 48 * sh, 0, WHITE);
	DrawTextEx(aller_b, (std::to_string(results.katu) + "x").c_str(), { 450.0f * sh, 336.0f * sh }, 48 * sh, 0, WHITE);
	DrawTextEx(aller_b, (std::to_string(results.misses) + "x").c_str(), { 450.0f * sh, 416.0f * sh }, 48 * sh, 0, WHITE);

	DrawTexturePro(atlas, tex[(int)SPRITE::Result300], { 72 * sh - tex[(int)SPRITE::Result300].width * sh / 2, 278 * sh - tex[(int)SPRITE::Result300].height * sh / 2, tex[(int)SPRITE::Result300].width * sh, tex[(int)SPRITE::Result300].height * sh }, { 0, 0 }, 0.0f, WHITE);
	DrawTexturePro(atlas, tex[(int)SPRITE::Result300g], { 402 * sh - tex[(int)SPRITE::Result300g].width * sh / 2, 278 * sh - tex[(int)SPRITE::Result300g].height * sh / 2, tex[(int)SPRITE::Result300g].width * sh, tex[(int)SPRITE::Result300g].height * sh }, { 0, 0 }, 0.0f, WHITE);
	DrawTexturePro(atlas, tex[(int)SPRITE::Result100], { 72 * sh - tex[(int)SPRITE::Result100].width * sh / 2, 358 * sh - tex[(int)SPRITE::Result100].height * sh / 2, tex[(int)SPRITE::Result100].width * sh, tex[(int)SPRITE::Result100].height * sh }, { 0, 0 }, 0.0f, WHITE);
	DrawTexturePro(atlas, tex[(int)SPRITE::Result100k], { 402 * sh - tex[(int)SPRITE::Result100k].width * sh / 2, 358 * sh - tex[(int)SPRITE::Result100k].height * sh / 2, tex[(int)SPRITE::Result100k].width * sh, tex[(int)SPRITE::Result100k].height * sh }, { 0, 0 }, 0.0f, WHITE);
	DrawTexturePro(atlas, tex[(int)SPRITE::Result50], { 72 * sh - tex[(int)SPRITE::Result50].width * sh / 2, 438 * sh - tex[(int)SPRITE::Result50].height * sh / 2, tex[(int)SPRITE::Result50].width * sh, tex[(int)SPRITE::Result50].height * sh }, { 0, 0 }, 0.0f, WHITE);
	DrawTexturePro(atlas, tex[(int)SPRITE::Result0], { 402 * sh - tex[(int)SPRITE::Result0].width * sh / 2, 438 * sh - tex[(int)SPRITE::Result0].height * sh / 2, tex[(int)SPRITE::Result0].width * sh, tex[(int)SPRITE::Result0].height * sh }, { 0, 0 }, 0.0f, WHITE);

	if (results.perfect_combo)
		DrawTexturePro(atlas, tex[(int)SPRITE::PerfectComboText], { 150.0f * sh, 616.0f * sh, tex[(int)SPRITE::PerfectComboText].width * sh, tex[(int)SPRITE::PerfectComboText].height * sh }, { 0, 0 }, 0.0f, WHITE);

	DrawTexturePro(atlas, rank_to_draw, { screen_width - 356.0f * sh, 120.0f * sh, 2 * rank_to_draw.width * sh, 2 * rank_to_draw.height * sh }, { 0, 0 }, 0.0f, WHITE);

	DrawRectangleV({ 220 * sh, 620 * sh }, { 240 * sh, 84 * sh }, { 0, 0, 0, 210 });
	DrawTextEx(aller_r, ur_text.c_str(), { 220 * sh, 620 * sh }, 20 * sh, 0, WHITE);

	// Draw header
	DrawRectangleV({ 0, 0 }, { screen_width, screen_height / 8 }, Color{ 0, 0, 0, 200 });
	DrawTextEx(aller_l, played_text.c_str(), { 4 * sh, 64 * sh}, 24 * sh, 0, WHITE);
	DrawTextEx(aller_l, beatmap_header.c_str(), { 4 * sh, 4 * sh}, 36 * sh, 0, WHITE);
	DrawTextEx(aller_l, beatmap_header_2.c_str(), { 4 * sh, 40 * sh }, 24 * sh, 0, WHITE);
	DrawTextEx(aller_r, "Ranking", { screen_width - 356.0f * sh, 4 * sh}, 108 * sh, 0, WHITE);
}

void result_screen::update() {
	if(IsKeyPressed(KEY_B)) {
		stop_sound_effect(channel_music);
		delete g_result_screen;
		g_result_screen = nullptr;
		song_select::init(true);
	}
}

void result_screen::save_score() const
{
	auto path = db::fs_path / "maps" / std::to_string(results.set_id) / std::to_string(results.map_id);
	if (!std::filesystem::is_directory(path)) {
		std::filesystem::create_directory(path);
	}

	std::ofstream file(path / (std::to_string(results.time.time_since_epoch().count()) + ".score"), std::ios::binary);
	if (!file) {
		std::cout << "Failed to create score file!";
		return;
	}

	// allocate 512 bytes per score just in case we need to add something later on
	const auto n = 512 - sizeof(results);
	char buffer[n] = { 0 };

	file.write((char*)(&results), sizeof(results));
	file.write(buffer, n);

	file.close();
}
