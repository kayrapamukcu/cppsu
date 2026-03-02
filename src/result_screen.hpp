#pragma once

#include "globals.hpp"

class result_screen {
public:
	result_screen(results_struct results);
	void draw();
	void update();
	void save_score() const;
private:
	results_struct results;
	std::string beatmap_header;
	std::string beatmap_header_2;
	std::string score_str;
	std::string played_text;
	int channel_music;
	Rectangle rank_to_draw;
	std::string ur_text;
};