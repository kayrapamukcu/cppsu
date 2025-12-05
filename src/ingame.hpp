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
	int repeat_count;
	bool head_hit;
	bool tail_hit;
	bool head_hit_checked;
	bool tail_hit_checked;
	float length;
	int repeat_left;
	Vector2 slider_ball_pos;
	std::vector<Vector3> path;
	std::vector<Vector3> corners;
	std::vector<Vector4> slider_ticks; // x, y, time, visible (0 = drawn, 1 = not drawn, 2 = reverse arrow, 4,5,6 = hit)
	uint32_t on_slider_tick;
	uint32_t slider_end_check_time;
	uint32_t total_hits;
	uint32_t successful_hits;
	bool tracked;
	unsigned char slider_type; // 0 = linear, 1 = bezier, 2 = perfect
};

struct Spinner {
	int rotation_requirement;
	float current_rotation;
	double max_acceleration;
	float rpm;
	double velocity;
	float rotation_count;
	int scoring_rotation_count;
	int bonus_score;
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
	void update();
	void draw();
private:
	inline void object_hit(Vector2 pos, HitResult res, bool is_slider);
	inline void recalculate_score_acc(HitResult res);
	void check_hit(bool notelock_check);
	static constexpr float slider_body_hit_radius = 2.0f;
	static constexpr float draw_hit_time = 0.4f;
	float map_speed = 1.0f;
	float mod_score_multiplier = 1.0f;
	float difficulty_multiplier = 1.0f;
	Vector2 mouse_pos = { 0, 0 };
	Vector2 mouse_pos_prev = { 0, 0 };
	file_struct map_info;
	float map_time = -1000.0f;
	float map_begin_time = -1000.0f;
	int map_first_object_time = 0;
	int on_object = 0;
	int visible_end = 0;

	float circle_radius = 200.0f;
	float slider_resolution = 4.0f;
	float slider_draw_resolution = slider_resolution * 2.5f * screen_scale;
	float accuracy = 100.0f;
	float tick_draw_delay = 40.0f;
	float spinner_rotation_ratio = 0.0f;

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
	std::vector<Spinner> spinners;
	std::deque<HitObjectResult> hits;

	bool key1_down = false;
	bool key2_down = false;
	float accumulated_end_time = 0;

	static constexpr int slider_repeat_score = 30;
	static constexpr int slider_tick_score = 10;
	static constexpr int spinner_spin_score = 100;
	static constexpr int spinner_bonus_score = 1100;
};