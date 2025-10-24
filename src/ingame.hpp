#pragma once

#include "globals.hpp"
#include <vector>
#include <array>

enum HitObjectType {
	CIRCLE,
	SLIDER,
	SPINNER
};

struct Circle {
	float x,y;
	uint8_t state; // 0 = active, 1 = 300, 2 = 100, 3 = 50, 4 = miss
	uint8_t color_idx;
};

struct Slider {
	float x, y;
	uint8_t state;
	uint8_t color_idx;
	std::vector<int> points;
};

struct Spinner {
	int time;
};

struct HitObjectEntry {
	int time;
	HitObjectType type;
};

struct TimingPoints {
	int time;
};

class ingame {
public:
	ingame(file_struct map);
	static void update();
	static void draw();
private:
	file_struct map_info;
	float mapTime = -1000.0f;
	int combo = 0;
	int maxCombo = 0;
	float acc = 100.0f;
	int hit300s = 0, hit100s = 0, hit50s = 0, misses = 0;
	std::array<Color, 8> hitColors = { // Green, Blue, Red, Yellow defaults
		Color{ 0, 255, 0, 255 },
		Color{ 0, 0, 255, 255 },
		Color{ 255, 0, 0, 255 },
		Color{ 255, 255, 0, 255 },
		Color{ 0, 0, 0, 255 },
		Color{ 0, 0, 0, 255 },
		Color{ 0, 0, 0, 255 },
		Color{ 0, 0, 0, 255 }
	};
	int approachRateMilliseconds = 450;
	int hitWindow300 = 200;
	int hitWindow100 = 200;
	int hitWindow50 = 200;

	std::vector<TimingPoints> timing_points;

	std::vector<HitObjectEntry> hit_objects;
	
	std::vector<Circle> circles;
	std::vector<Slider> sliders;
	std::vector<Spinner> spinners;
};