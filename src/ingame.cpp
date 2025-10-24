#include "ingame.hpp";
#include <fstream>

ingame::ingame(file_struct map) {
	map_info = map;
	hitWindow300 = (int)(80.0f - 6.0f * map_info.od);
	hitWindow100 = (int)(140.0f - 8.0f * map_info.od);
	hitWindow50 = (int)(200.0f - 10.0f * map_info.od);
	approachRateMilliseconds = (map_info.ar < 5.0f) ? int(1800 - 120 * map_info.ar) : int(1200 - 150 * (map_info.ar - 5));

	std::string line;
	std::ifstream infile(map.osu_filename);
	while (std::getline(infile, line)) {
		chomp_cr(line);
		// Parse hit objects and timing points here
	}
}

void ingame::update() {
	return;
}

void ingame::draw() {
	return;
}