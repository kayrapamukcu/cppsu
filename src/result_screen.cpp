#include "result_screen.hpp"
#include "song_select.hpp"
#include <format>

result_screen::result_screen(results_struct results)
{
	this->results = results;
	StopMusicStream(music);
}

void result_screen::draw() {
	
	DrawTextureCompatPro(background, { 0,0, screen_width, screen_height }, WHITE);
	// Draw overlay
	DrawRectangle(0, 0, (int)screen_width, (int)(screen_height/8), Color{ 0, 0, 0, 200 });
	DrawTextEx(font36, results.beatmap_header.c_str(), {4, 4}, 36, 0, WHITE);
	DrawTextEx(font24, results.beatmap_header_2.c_str(), { 4, 40 }, 24, 0, WHITE);
	DrawTextEx(font36, "Ranking", { screen_width - 32 - MeasureTextEx(font36, "Ranking", 96, 0).x, 4}, 96, 0, WHITE);

	// todo : precompute time
	auto time = std::chrono::zoned_time{ std::chrono::current_zone(), results.time };
	DrawTextEx(font24, ("Played by " + results.player_name + " on " + std::format("{:%d.%m.%Y %H:%M:%S}", time) + ".").c_str(), {4, 64}, 24, 0, WHITE);

	// todo : move these into init
	DrawRectangleGradientV(16, screen_height / 6, screen_width / 2.4, screen_height / 12, Color{ 228, 206, 167, 255 }, Color{ 178, 157, 138, 255 });
	auto score_str = std::to_string(results.score);
	auto n = score_str.length();
	if (n < 8) score_str.insert(0, 8 - n, '0');
	DrawTextEx(font36_b, score_str.c_str(), {144, 128}, 72, 0, WHITE);

	DrawRectangleGradientV(16, screen_height / 3.8, screen_width / 2.4, screen_height / 1.92, Color{ 53, 128, 219, 255 }, Color{ 35, 91, 165, 255 });

}

void result_screen::update() {
	if(IsKeyPressed(KEY_B)) {
		delete g_result_screen;
		g_result_screen = nullptr;
		song_select::init(true);
	}
}