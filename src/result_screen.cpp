#include "result_screen.hpp"
#include "song_select.hpp"
#include <format>

result_screen::result_screen(results_struct results)
{
	this->results = results;
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

	played_text = "Played by " + results.player_name + " on " + std::string(time_buf) + ".";

	channel_music = play_sound_effect("applause.mp3");
	switch (results.rank) {
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

	DrawTexturePro(atlas, rank_to_draw, { screen_width - 344.0f * sh, 120.0f * sh, 10 * rank_to_draw.width * sh, 10 * rank_to_draw.height * sh }, { 0, 0 }, 0.0f, WHITE);

	// Draw header
	DrawRectangle(0, 0, (int)screen_width, (int)(screen_height / 8), Color{ 0, 0, 0, 200 });
	DrawTextEx(aller_l, played_text.c_str(), { 4 * sh, 64 * sh}, 24 * sh, 0, WHITE);
	DrawTextEx(aller_l, results.beatmap_header.c_str(), { 4 * sh, 4 * sh}, 36 * sh, 0, WHITE);
	DrawTextEx(aller_l, results.beatmap_header_2.c_str(), { 4 * sh, 40 * sh }, 24 * sh, 0, WHITE);
	DrawTextEx(aller_r, "Ranking", { screen_width - 344.0f * sh, 4 * sh}, 108 * sh, 0, WHITE);
}

void result_screen::update() {
	if(IsKeyPressed(KEY_B)) {
		stop_sound_effect(channel_music);
		delete g_result_screen;
		g_result_screen = nullptr;
		song_select::init(true);
	}
}