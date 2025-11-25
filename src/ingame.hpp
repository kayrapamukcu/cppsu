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
	float time_remaining;
	HitResult result;
};

struct Slider {
	uint16_t repeat_count;
	bool head_hit;
	unsigned char slider_type; // 0 = linear, 1 = bezier, 2 = perfect
	float length;
	std::vector<Vector3> path;
	std::vector<Vector3> corners;
	int repeat_left;
	Vector2 slider_ball_pos;
};

struct HitObjectEntry {
	Vector2 pos;
	int time;
	int end_time;
	uint32_t idx;
	uint32_t color_idx;
	HitObjectType type;
	uint32_t combo_idx;
};

struct TimingPoints {
	int time;
	float slider_velocity;
	float ms_beat;
	int volume;
};

class ingame {
public:
	ingame(file_struct map);
	void check_hit();
	void update();
	void draw();
	inline void recalculate_acc();
private:
	static constexpr float draw_hit_time = 0.4f;
	float map_speed = 1.0f;
	Vector2 mouse_pos = { 0, 0 };
	file_struct map_info;
	float map_time = -1000.0f;
	float map_begin_time = -1000.0f;
	int map_first_object_time = 0;
	int on_object = 0;
	int visible_end = 0;

	float circle_radius = 200.0f;
	float slider_resolution = 6.0f;
	float accuracy = 100.0f;
	uint32_t score = 0;
	uint32_t combo = 0;
	uint32_t max_combo = 0;
	uint32_t hit300s = 0, hit100s = 0, hit50s = 0, misses = 0;
	std::array<Color, 8> hit_colors = { // Green, Blue, Red, Yellow defaults
		Color{ 255, 192, 0, 255 },
		Color{ 0, 202, 0, 255 },
		Color{ 18, 124, 255, 255 },
		Color{ 242, 24, 57, 255 },
		Color{ 0, 0, 0, 255 },
		Color{ 0, 0, 0, 255 },
		Color{ 0, 0, 0, 255 },
		Color{ 0, 0, 0, 255 }
	};

	int approach_rate_milliseconds = 450;
	int hit_window_300 = 200;
	int hit_window_100 = 200;
	int hit_window_50 = 200;

	uint32_t song_init = 0;
	uint8_t hit_color_count = 0;
	uint8_t hit_color_current = 0;
	bool skippable = false;

	std::vector<TimingPoints> timing_points;
	std::vector<HitObjectEntry> hit_objects;
	std::vector<Slider> sliders;
	std::deque<HitObjectResult> hits;

	bool key1_down = false;
	bool key2_down = false;
	float accumulated_end_time = 0;
};