#include "ingame.hpp";
#include <fstream>
#include <filesystem>
#include <iostream>
#include <rlgl.h>
#include "result_screen.hpp"
#include "globals.hpp"

ingame::ingame(file_struct map) {
	// TODO : IMPLEMENT STACK LENIENCY
	map_info = map;
	hit_window_300 = (int)(80.0f - 6.0f * map_info.od);
	hit_window_100 = (int)(140.0f - 8.0f * map_info.od);
	hit_window_50 = (int)(200.0f - 10.0f * map_info.od);
	approach_rate_milliseconds = (map_info.ar < 5.0f) ? int(1800 - 120 * map_info.ar) : int(1200 - 150 * (map_info.ar - 5));

	circle_radius = playfield_scale * (50 - (4.0f * map_info.cs));

	// parse .osu file	
	auto file = std::filesystem::current_path() / "maps" / std::to_string(map.beatmap_set_id) / map.osu_filename;
	std::string line;
	std::ifstream infile(file);
	if(!infile.is_open()) {
		std::cout << "Failed to open map file: " << file << "\n";
		return;
	}
	std::cout << "Opening: " << file.string() << "\n";
	int state = 0; // 0 = before timing points, 1 = timing points, 2 = colours
    float current_bpm = 0;
	float slider_mult = 0;

	uint16_t circle_cnt = 0;
	uint16_t slider_cnt = 0;
	uint16_t spinner_cnt = 0;

    while (std::getline(infile, line)) {
        chomp_cr(line);
		if (line == "[HitObjects]") {
			break;
		}
        if (state < 1) {
            if (line == "[TimingPoints]") state = 1;
            continue;
        }
        else if (state == 1) {
			if (line.empty()) continue;
			if (line == "[Colours]") {
				state = 2;
				continue;
			}
			std::string_view sv = line;
			std::array<std::string_view, 6> parts{};
			size_t start = 0;
			size_t pos = 0;
			int index = 0;
			while (index < 6) {
				size_t pos = sv.find(',', start);
				parts[index++] = sv.substr(start, pos - start);
				start = pos + 1;
			}

			// parts[5] = sv.substr(start, pos - start);

			float ms;
			to_float(parts[1], ms);
			if (ms > 0) { // uninherited timing point
				current_bpm = 60000.0f / ms;
				slider_mult = map_info.slider_multiplier * 1.0f;
			}
			else { // inherited timing point
				slider_mult = map_info.slider_multiplier * -100.0f / ms;
			}

			int volume;
			to_int(parts[5], volume);
			timing_points.push_back(TimingPoints{ std::stoi(std::string(parts[0])), slider_mult, (uint8_t)volume });
        }
        else if (state == 2){
            if (line.starts_with("Combo")) {
				size_t idx = line.find(":");
				size_t first_comma = line.find(",", idx);
				size_t second_comma = line.find(",", first_comma + 1);
				int color_idx = std::stoi(line.substr(5, 1), nullptr);
				hit_colors[color_idx - 1] = Color{ (unsigned char)std::stoi(line.substr(idx + 2, first_comma - (idx + 2))),(unsigned char)std::stoi(line.substr(first_comma + 1, second_comma - (first_comma + 1))),(unsigned char)std::stoi(line.substr(second_comma + 1)),255 };
				hit_color_count++;
            }
        }

        
    }
	if (hit_color_count < 2) hit_color_count = 4;
	timing_points.shrink_to_fit();
	int timing_point_idx = 0;
	while (std::getline(infile, line)) {
		chomp_cr(line);
		if (line.empty()) continue;
		std::string_view sv = line;
		std::array<std::string_view, 5> parts;
		size_t start = 0;
		size_t pos = 0;
		int index = 0;
		while (index < 5) {
			size_t pos = sv.find(',', start);
			parts[index++] = sv.substr(start, pos - start);
			start = pos + 1;
		}
		int time, type;
		to_int(parts[2], time);
		to_int(parts[3], type);

		HitObjectType hot = CIRCLE;
		if (type & 2) hot = SLIDER;
		else if (type & 8) hot = SPINNER;
		if (type & 4) {
			hit_color_current++;
			if (type & 16) hit_color_current++;
			if (type & 32) hit_color_current+=2;
			if (type & 64) hit_color_current+=4;
		}
		
		hit_color_current = hit_color_current % hit_color_count;

		switch (hot) {
		case CIRCLE: {

			float x, y;
			to_float(parts[0], x);
			to_float(parts[1], y);
			x *= playfield_scale;
			y *= playfield_scale;
			x += playfield_offset_x;
			y += playfield_offset_y;
			hit_objects.push_back(HitObjectEntry{ {x, y}, time, time, circle_cnt, hit_color_current, hot });
			circle_cnt++;
			break;
		}
		case SLIDER: {
			int end_time;
			// find correct timing point
			while (timing_point_idx + 1 < timing_points.size() && timing_points[timing_point_idx + 1].time <= time) {
				timing_point_idx++;
			}
			// parse additional fields
			std::array<std::string_view, 6> slider_parts{};
			index = 0;
			while (index < 6 && sv.find(',', start != std::string::npos)) {
				size_t pos = sv.find(',', start);
				slider_parts[index++] = sv.substr(start, pos - start);
				start = pos + 1;
			}
			int slider_repeat, slider_length;
			to_int(slider_parts[1], slider_repeat);
			to_int(slider_parts[2], slider_length);
			end_time = time + (int)(slider_repeat * slider_length * timing_points[timing_point_idx].slider_velocity);
			float x, y;
			to_float(parts[0], x);
			to_float(parts[1], y);
			x *= playfield_scale;
			y *= playfield_scale;
			x += playfield_offset_x;
			y += playfield_offset_y;
			// parse slider shape
			auto& slider_line = slider_parts[0];
			unsigned char shape = slider_line[0];
			std::vector<Vector2> points;
			slider_line.remove_prefix(2);

			while (!slider_line.empty()) {
				size_t delim = slider_line.find('|');
				std::string_view pair = slider_line.substr(0, delim);

				if (!pair.empty()) {
					size_t colon = pair.find(':');
					if (colon != std::string_view::npos) {
						float x, y;
						to_float(pair.substr(0, colon), x);
						to_float(pair.substr(colon + 1), y);
						points.push_back({ x * playfield_scale + playfield_offset_x, y * playfield_scale + playfield_offset_y });
					}
				}

				if (delim == std::string_view::npos) break;
				slider_line.remove_prefix(delim + 1);
			}

			sliders.push_back(Slider{ (uint16_t) slider_repeat, false, shape, std::move(points)});
			hit_objects.push_back(HitObjectEntry{ {x, y}, time, end_time, slider_cnt, hit_color_current, hot });
			slider_cnt++;
			break;
		}
		case SPINNER: 
			int end_time;
			to_int(sv.substr(start), end_time);
			hit_objects.push_back(HitObjectEntry{ {256 * playfield_scale + playfield_offset_x, 192 * playfield_scale + playfield_offset_y}, time, end_time, spinner_cnt, hit_color_current, hot });
			spinner_cnt++;
			break;  
		default:
			break;
		}
	}
	map_begin_time = -GetTime() * 1000.0f * map_speed - 1000.0f;
	
	hit_objects.shrink_to_fit();
	sliders.shrink_to_fit();

	map_first_object_time = hit_objects[0].time;
}

void ingame::recalculate_acc() {
	uint32_t total_hits = hit300s + hit100s + hit50s + misses;
	if (total_hits == 0) {
		accuracy = 100.0f;
		return;
	}
	accuracy = (float)(hit300s * 300 + hit100s * 100 + hit50s * 50) / (float)(total_hits * 300) * 100.0f;

	// also calculate score
	score += 300 * combo;
}

void ingame::check_hit() {
	auto& ho = hit_objects[on_object];
	float delta = std::abs(map_time - ho.time);
	if (delta > 360) return; // way too early

	Vector2 mouse_pos = { (float)GetMouseX(), (float)GetMouseY() };
	switch (ho.type) {
	case CIRCLE: {
		
		if (CheckCollisionPointCircle(mouse_pos, ho.pos, circle_radius)) {
			if (delta <= hit_window_300) {
				std::cout << "300 with offset " << map_time - ho.time << "!\n";
				hit300s++;
				combo++;
				if (max_combo < combo) max_combo = combo;
				hits.push_back({ ho.pos, draw_hit_time, HIT_300});
			}
			else if (delta <= hit_window_100) {
				std::cout << "100 with offset " << map_time - ho.time << "!\n";
				hit100s++;
				combo++;
				if (max_combo < combo) max_combo = combo;
				hits.push_back({ ho.pos, draw_hit_time, HIT_100 });
			}
			else if (delta <= hit_window_50) {
				std::cout << "50 with offset " << map_time - ho.time << "!\n";
				hit50s++;
				combo++;
				if (max_combo < combo) max_combo = combo;
				hits.push_back({ ho.pos, draw_hit_time, HIT_50 });
			}
			else {
				std::cout << "miss with offset " << map_time - ho.time << "!\n";
				misses++;
				if (max_combo < combo) max_combo = combo;
				combo = 0;
				hits.push_back({ ho.pos, draw_hit_time, MISS });
			}
			on_object++;
			recalculate_acc();
		}
		break;
	}
	case SLIDER: {
		auto& s = sliders[ho.idx];
		delta = std::abs(map_time - ho.time);
		if (!s.head_hit) {
			delta = std::abs(map_time - ho.time);
			if (delta > 360) return;
			if (CheckCollisionPointCircle(mouse_pos, ho.pos, circle_radius)) {
				if (delta <= hit_window_50) {
					std::cout << "Slider head hit! " << map_time - ho.time << "!\n";
					combo++;
					if (max_combo < combo) max_combo = combo;
				}
				else {
					std::cout << "Slider head miss! with offset " << map_time - ho.time << "!\n";
					if (max_combo < combo) max_combo = combo;
					combo = 0;
				}
			}
		}
		break;
		}
	}
}

void ingame::update() {
	map_time = map_begin_time + GetTime() * 1000.0f * map_speed;

	if (on_object >= (int)hit_objects.size()) { // Check for end of map
		accumulated_end_time += GetFrameTime();

		if (hit_objects[hit_objects.size() - 1].time > GetMusicTimePlayed(music) * 1000.0f + 500) {
			StopMusicStream(music);
		}

		if (accumulated_end_time > 2.0f || IsKeyPressed(KEY_ESCAPE)) {
			results_struct res;
			res.time = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now());
			res.beatmap_header = map_info.artist + " - " + map_info.title + " [" + map_info.difficulty + "]";
			res.beatmap_header_2 = "Beatmap by " + map_info.creator;
			res.player_name = player_name;
			res.accuracy = accuracy;
			res.score = score;
			res.max_combo = max_combo;
			res.hit300s = hit300s;
			res.hit100s = hit100s;
			res.hit50s = hit50s;
			res.misses = misses;
			// calculate rank
			ranks rank = RANK_D;
			int all_objects = hit300s + hit100s + hit50s + misses;
			if (accuracy == 100.0f) rank = RANK_X;
			if (hit300s >= all_objects * 0.9) {
				if (misses == 0 && hit50s <= all_objects * 0.01f) rank = RANK_S;
				else rank = RANK_A;
			}
			if (hit300s >= all_objects * 0.8f) {
				if (misses == 0) rank = RANK_A;
				rank = RANK_B;
			}
			if (hit300s >= all_objects * 0.6f) {
				if (misses == 0 && hit300s >= all_objects * 0.7f) rank = RANK_B;
				rank = RANK_C;
			}
			res.rank = rank;

			g_result_screen = new result_screen(res);
			delete g_ingame;
			g_ingame = nullptr;
			game_state = RESULT_SCREEN;
			return;
		}
	}

	if (IsKeyPressed(KEY_Z)) {
		std::cout << "map begin time: " << map_begin_time << " map time: " << map_time << " music time: " << GetMusicTimePlayed(music) << " first object time: " << map_first_object_time << "\n";
	}
	switch (song_init) {
	case 0: // song not initialized
		if (map_time >= 0.0f) {
			map_time = 0.0f;
			PlayMusicStream(music);
			if(map_speed != 1.0f) {
				SetMusicPitch(music, map_speed);
			}
			song_init = 1;
			if (map_first_object_time - 3000.0f > map_time)
				skippable = true;
			else song_init = 2;
		}	
		break;
	case 1: // song started playing, but map not yet begun
		if (map_time >= map_first_object_time - 2500.0f) {
			skippable = false;
			song_init = 2;
		}
		if (skippable && IsKeyPressed(KEY_SPACE)) {
			std::cout << "skipping\n";
			float skip_target_time = map_first_object_time - 2500.0f;

			SeekMusicStream(music, skip_target_time / 1000.0f);
			UpdateMusicStream(music);

			map_begin_time = skip_target_time - GetTime() * 1000.0f * map_speed;

			skippable = false;
			song_init = 2;
		}
		break;
	case 2: // map ongoing
		if (IsKeyPressed(k_1)) {
			k1_down = true;
			check_hit();
		}
		if (IsKeyPressed(k_2)) {
			k2_down = true;
			check_hit();
		}
		if (IsKeyReleased(k_1)) {
			k1_down = false;
		}
		if (IsKeyReleased(k_2)) {
			k2_down = false;
		}

		if (k1_down || k2_down) {
			// implement slider & spinner behavior
		}

		break;
	}

	while (on_object < (int)hit_objects.size() && hit_objects[on_object].end_time < map_time - hit_window_50 * 0.5f) {
		// missed object
		auto& ho = hit_objects[on_object];
		std::cout << "missed object at time " << ho.time << "\n";
		misses++;
		combo = 0;
		hits.push_back({ ho.pos, draw_hit_time, MISS });
		on_object++;
		recalculate_acc();
	}

	while (visible_end < (int)hit_objects.size() && hit_objects[visible_end].time - approach_rate_milliseconds <= map_time) {
		visible_end++;
	}

}

void ingame::draw() {
	ClearBackground(BLACK);
	DrawRectangleLines(playfield_offset_x, playfield_offset_y, playfield_scale * 512, playfield_scale * 384, WHITE);
	
	// --- Draw objects ---

	for (int i = visible_end - 1; i >= on_object; i--) {
		auto& ho = hit_objects[i];

		unsigned char alpha = (unsigned char)std::clamp(255.0f * ((map_time - (ho.time - approach_rate_milliseconds)) / (0.666f * approach_rate_milliseconds)), 0.0f, 255.0f);
		switch (ho.type) {
		case CIRCLE: {
			float approach_scale = std::max(0.0f, (ho.time - map_time)) / approach_rate_milliseconds * circle_radius * 3 + circle_radius;
			Color color = hit_colors[ho.color_idx];
			color.a = alpha;
			DrawCircleLinesV(ho.pos, approach_scale, color);
			DrawCircleV(ho.pos, circle_radius, color);
			break;
		}
		case SLIDER: {
			auto& s = sliders[ho.idx];
			float approach_scale = std::max(0.0f, (ho.time - map_time)) / approach_rate_milliseconds * circle_radius * 3 + circle_radius;
			Color color = hit_colors[ho.color_idx];
			color.a = alpha;
			DrawCircleLinesV(ho.pos, approach_scale, color);

			Color body_color = { 80, 80, 80, 0.5f * alpha };
			switch (s.slider_type) { // TODO: Learn math bro...
			case 'L': {
				DrawLineEx(ho.pos, s.points[0], circle_radius * 2, body_color);
				break;
			}
			case 'P': {
				break;
			}
			case 'C':
			case 'B': {
				if (s.points.size() == 1) {
					DrawLineEx(ho.pos, s.points[0], circle_radius * 2, body_color);
					break;
				}
				// DrawSplineBezierQuadratic(s.points.data(), (int)s.points.size(), circle_radius*2, body_color);
				break;
			}
			default:
				break;
			}
			DrawCircleV(ho.pos, circle_radius, color);
			DrawCircleV(s.points[s.points.size() - 1], circle_radius, color);

			break;
		}
		case SPINNER: {
			// spinners always have an approach rate of 400ms from my testing
			alpha = (unsigned char)std::clamp(255.0f * ((map_time - (ho.time - 400)) / (400)), 0.0f, 255.0f);
			float x, y;
			x = 256 * playfield_scale + playfield_offset_x;
			y = 192 * playfield_scale + playfield_offset_y;
			DrawCircleV(Vector2{ x, y }, 256 * playfield_scale, { 64, 128, 255, alpha });
			auto radius = (map_time < ho.time) ? 256 * playfield_scale : 240 * playfield_scale * (ho.end_time - map_time) / (ho.end_time - ho.time) + 16;
			DrawCircleLinesV(Vector2{ x, y }, radius, { 255, 255, 255, alpha });
			DrawCircleV(Vector2{ x, y }, 16, { 255, 255, 255, alpha });
			break;
		}
		}
	}

	// --- Draw hits ---
	float frame_time = GetFrameTime();
	for (auto it = hits.begin(); it != hits.end(); ) {
		auto& h = *it;
		// todo: precompute text width or use textures
		switch (h.result) {
		case HIT_300:
			DrawTextEx(font24, "300", { h.pos.x - MeasureTextEx(font24, "300", 24, 0).x / 2.0f, h.pos.y }, 24, 0, BLUE);
			break;
		case HIT_100:
			DrawTextEx(font24, "100", { h.pos.x - MeasureTextEx(font24, "100", 24, 0).x / 2.0f, h.pos.y }, 24, 0, GREEN);
			break;
		case HIT_50:
			DrawTextEx(font24, "50", { h.pos.x - MeasureTextEx(font24, "50", 24, 0).x / 2.0f, h.pos.y }, 24, 0, YELLOW);
			break;
		case MISS:
			DrawTextEx(font24, "Miss!", { h.pos.x - MeasureTextEx(font24, "Miss!", 24, 0).x / 2.0f, h.pos.y }, 24, 0, RED);
			break;
		}

		h.time_remaining -= frame_time;

		if (h.time_remaining <= 0.0f) {
			it = hits.erase(it);
		}
		else {
			++it;
		}
	}
	return;
}