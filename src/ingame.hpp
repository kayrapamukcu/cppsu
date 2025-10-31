#pragma once

#include "globals.hpp"
#include <vector>
#include <array>
#include <deque>

enum HitObjectType {
	CIRCLE,
	SLIDER,
	SPINNER
};

enum HitResult {
	HIT_300,
	HIT_100,
	HIT_50,
	MISS
};

struct HitObjectResult {
	Vector2 pos;
	HitResult result;
	float time_remaining;
};

struct Slider {
	uint8_t repeat_count;
	bool head_hit;
	unsigned char slider_type; // 0 = linear, 1 = bezier, 2 = perfect
	std::vector<Vector2> points;
	std::vector<Vector2> path;
};

struct HitObjectEntry {
	HitObjectType type;
	Vector2 pos;
	int time;
	int end_time;
	uint16_t idx;
	uint8_t color_idx;
};

struct TimingPoints {
	int time;
	float slider_velocity;
	uint8_t volume;
};

class ingame {
public:
	ingame(file_struct map);
	void check_hit();
	void update();
	void draw();
private:
	const float map_speed = 1.0f;

	file_struct map_info;
	float map_time = -1000.0f;
	float map_begin_time = -1000.0f;
	int combo = 0;
	int max_combo = 0;
	float acc = 100.0f;
	int hit300s = 0, hit100s = 0, hit50s = 0, misses = 0;
	std::array<Color, 8> hit_colors = { // Green, Blue, Red, Yellow defaults
		Color{ 0, 255, 0, 255 },
		Color{ 0, 0, 255, 255 },
		Color{ 255, 0, 0, 255 },
		Color{ 255, 255, 0, 255 },
		Color{ 0, 0, 0, 255 },
		Color{ 0, 0, 0, 255 },
		Color{ 0, 0, 0, 255 },
		Color{ 0, 0, 0, 255 }
	};
	std::deque<HitObjectResult> hits;
	uint8_t hit_color_count = 0;
	uint8_t hit_color_current = 0;
	int approach_rate_milliseconds = 450;
	int hit_window_300 = 200;
	int hit_window_100 = 200;
	int hit_window_50 = 200;

	float circle_radius = 200.0f;

	int map_first_object_time = 0;
	int on_object = 0;
	
	bool skippable = false;
	uint8_t song_init = 0;
	std::vector<TimingPoints> timing_points;

	std::vector<HitObjectEntry> hit_objects;
	
	std::vector<Slider> sliders;

	bool key1_down = false;
	bool key2_down = false;
	float accumulated_end_time = 0;
};