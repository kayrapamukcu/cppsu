#pragma once

#include "globals.hpp"

class result_screen {
public:
	result_screen(results_struct results, file_struct map);
	void draw();
	void update();
	void callback_play_replay(int);
private:
	results_struct results;
	std::string beatmap_header;
	std::string beatmap_header_2;
	std::string score_str;
	std::string played_text;
	int channel_music;
	Rectangle rank_to_draw;
	std::string ur_text;
	bool replay_exists = false;
	button_callback replay_button;
	file_struct map;
};