#include "result_screen.hpp"
#include "song_select.hpp"
#include <format>

result_screen::result_screen(results_struct results)
{
	this->results = results;
	StopMusicStream(music);
	score_str = std::to_string(results.score);
	auto n = score_str.length();
	if (n < 8) score_str.insert(0, 8 - n, '0');

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
}

void result_screen::draw() {
	
	DrawTextureCompatPro(background, { 0,0, screen_width, screen_height }, WHITE);
	
	DrawRectangleGradientV(16 * screen_width_ratio, screen_height / 6.0f, screen_width / 1.7f, screen_height / 12.0f, Color{ 228, 206, 167, 255 }, Color{ 178, 157, 138, 255 });
	DrawTextEx(aller_b, score_str.c_str(), {144, 128}, 72, 0, WHITE);

	DrawRectangleGradientV(16, screen_height / 3.8f, screen_width / 1.7f, screen_height / 1.92f, Color{ 53, 128, 219, 255 }, Color{ 35, 91, 165, 255 });
	DrawTextEx(aller_l, "Combo", { 18, screen_height / 1.54f }, 48, 0, WHITE);

	DrawTextEx(aller_b, (std::to_string(results.max_combo) + "x").c_str(), { 28, screen_height / 1.44f }, 60, 0, WHITE);

	DrawTextEx(aller_l, "Accuracy", { 18 + screen_width / 3.0f, screen_height / 1.54f }, 48, 0, WHITE);
	DrawTextEx(aller_b, (std::format("{:.2f}%", results.accuracy)).c_str(), { 28 + screen_width / 3.0f, screen_height / 1.44f }, 60, 0, WHITE);

	DrawTextEx(aller_b, (std::to_string(results.hit300s) + "x").c_str(), {screen_width / 8.5f, screen_height / 3.0f}, 48 * screen_scale, 0, WHITE);
	DrawTextEx(aller_b, (std::to_string(results.hit100s) + "x").c_str(), {screen_width / 8.5f, screen_height / 2.3f}, 48 * screen_scale, 0, WHITE);
	DrawTextEx(aller_b, (std::to_string(results.hit50s) + "x").c_str(), {screen_width / 8.5f, screen_height / 1.75f}, 48 * screen_scale, 0, WHITE);

	DrawTextEx(aller_b, (std::to_string(results.geki) + "x").c_str(), { screen_width / 2.3f, screen_height / 2.3f }, 48 * screen_scale, 0, WHITE);
	DrawTextEx(aller_b, (std::to_string(results.katu) + "x").c_str(), { screen_width / 2.3f, screen_height / 3.0f }, 48 * screen_scale, 0, WHITE);
	DrawTextEx(aller_b, (std::to_string(results.misses) + "x").c_str(), { screen_width / 2.3f, screen_height / 1.75f }, 48 * screen_scale, 0, WHITE);

	// Draw header
	DrawRectangle(0, 0, (int)screen_width, (int)(screen_height / 8), Color{ 0, 0, 0, 200 });
	DrawTextExScaled(aller_l, played_text.c_str(), { 4, 64 }, 24, 0, WHITE);
	DrawTextExScaled(aller_l, results.beatmap_header.c_str(), { 4, 4 }, 36, 0, WHITE);
	DrawTextExScaled(aller_l, results.beatmap_header_2.c_str(), { 4, 40 }, 24, 0, WHITE);
	DrawTextExScaled(aller_r, "Ranking", { 700, 4 }, 108, 0, WHITE);
}

void result_screen::update() {
	if(IsKeyPressed(KEY_B)) {
		delete g_result_screen;
		g_result_screen = nullptr;
		song_select::init(true);
	}
}