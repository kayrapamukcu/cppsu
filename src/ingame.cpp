#include "ingame.hpp";
#include <fstream>
#include <filesystem>
#include <iostream>
#include <rlgl.h>
#include "result_screen.hpp"
#include "globals.hpp"
#include "rlgl.h"

ingame::ingame(file_struct map) {
	// TODO : IMPLEMENT STACK LENIENCY
	map_info = map;
	hit_window_300 = (int)(80.0f - 6.0f * map_info.od);
	hit_window_100 = (int)(140.0f - 8.0f * map_info.od);
	hit_window_50 = (int)(200.0f - 10.0f * map_info.od);
	approach_rate_milliseconds = (map_info.ar < 5.0f) ? int(1800 - 120 * map_info.ar) : int(1200 - 150 * (map_info.ar - 5));

	circle_radius = playfield_scale * (54.4 - (4.48f * map_info.cs));

	// parse .osu file	
	auto file = std::filesystem::current_path() / "maps" / std::to_string(map.beatmap_set_id) / map.osu_filename;
	std::string line;
	std::ifstream infile(file);
	if (!infile.is_open()) {
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
			static float ms_beat = 500.0f;
			float ms;
			to_float(parts[1], ms);
			if (ms > 0) { // uninherited timing point
				current_bpm = 60000.0f / ms;
				ms_beat = ms;
				slider_mult = map_info.slider_multiplier * 1.0f;
			}
			else { // inherited timing point
				slider_mult = map_info.slider_multiplier * -100.0f / ms;
			}

			int volume;
			to_int(parts[5], volume);
			int offset;
			to_int(parts[0], offset);
			timing_points.push_back(TimingPoints{ offset, slider_mult, ms_beat, volume });
		}
		else if (state == 2) {
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
	uint32_t combo_idx = 0;
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

		combo_idx++;

		HitObjectType hot = CIRCLE;
		if (type & 2) hot = SLIDER;
		else if (type & 8) hot = SPINNER;
		if (type & 4) {
			hit_color_current++;
			if (type & 16) hit_color_current++;
			if (type & 32) hit_color_current += 2;
			if (type & 64) hit_color_current += 4;
			combo_idx = 1;
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
			hit_objects.push_back(HitObjectEntry{ {x, y}, time, time, circle_cnt, hit_color_current, hot, combo_idx });
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
			end_time = time + (int)(slider_repeat * slider_length * timing_points[timing_point_idx].ms_beat / (100 * timing_points[timing_point_idx].slider_velocity)); // TODO : is this correct?
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

			// CALCULATE SLIDER PATH!

			std::vector<Vector3> path;
			std::vector<Vector3> corners;

			switch (shape) {
			case 'L': {
				// linear slider
				auto angle = atan2f(points.back().y - y, points.back().x - x);
				auto x_mult = cosf(angle) * playfield_scale;
				auto y_mult = sinf(angle) * playfield_scale;
				path.push_back({ x + x_mult * slider_length, y + y_mult * slider_length, 90 + angle * 180.f / PI });
				break;
			}
			case 'P': {
				// perfect curve (circle arc) slider
				Vector2 p0 = { x, y };
				Vector2 p1 = points.at(0);
				Vector2 p2 = points.at(1);

				Vector2 center;
				float radius;

				float x1 = p0.x, y1 = p0.y;
				float x2 = p1.x, y2 = p1.y;
				float x3 = p2.x, y3 = p2.y;

				float a = p0.x * (y2 - y3) - p0.y * (p1.x - p2.x) + p1.x * y3 - p2.x * y2;

				float x1_sq = p0.x * p0.x + p0.y * p0.y;
				float x2_sq = p1.x * p1.x + p1.y * p1.y;
				float x3_sq = p2.x * p2.x + p2.y * p2.y;

				float bx = (x1_sq * (p1.y - p2.y) + x2_sq * (p2.y - p0.y) + x3_sq * (p0.y - p1.y));

				float by = (x1_sq * (p2.x - p1.x) + x2_sq * (p0.x - p2.x) + x3_sq * (p1.x - p0.x));

				float denom = 2.0f * a;

				center.x = bx / denom;
				center.y = by / denom;

				radius = std::sqrt((center.x - x1) * (center.x - x1) + (center.y - y1) * (center.y - y1));

				float start = std::atan2f(p0.y - center.y, p0.x - center.x);
				float mid = ShiftRadiansForward(std::atan2f(p1.y - center.y, p1.x - center.x), start);
				float end = ShiftRadiansForward(std::atan2f(p2.y - center.y, p2.x - center.x), start);

				if (!(mid > start && mid < end)) {
					std::swap(start, end);
				}

				float angleEnd = ShiftRadiansForward(end, start);
				float arcAngle = angleEnd - start;

				float arcLen = radius * arcAngle;
				int steps = std::max(2, (int)std::ceil(arcLen / slider_resolution));

				for (int i = 0; i <= steps; ++i) {
					float t = (float)i / (float)steps;
					float ang = start + t * arcAngle;
					Vector3 p;
					p.x = center.x + std::cosf(ang) * radius;
					p.y = center.y + std::sinf(ang) * radius;
					p.z = 180.0f * ang / PI;
					path.push_back(p);
				}

				float dx0 = path.at(0).x - p0.x;
				float dy0 = path.at(0).y - p0.y;
				float dist_from_start = dx0 * dx0 + dy0 * dy0;
				float dx1 = path.back().x - p0.x;
				float dy1 = path.back().y - p0.y;
				float dist_from_end = dx1 * dx1 + dy1 * dy1;
				// If the end of the arc is closer to p0 than the beginning, reverse the list
				if (dist_from_end < dist_from_start) {
					std::reverse(path.begin(), path.end());
				}

				break;
			}
			case 'C':
			case 'B': {
				auto bezier_point = [](const std::vector<Vector2>& controlPoints, float t) -> Vector2 { // get bezier point at t
					std::vector<Vector2> tmp = controlPoints;
					int n = tmp.size();
					for (int k = n - 1; k > 0; k--) {
						for (int i = 0; i < k; i++) {
							tmp[i].x = tmp[i].x + (tmp[i + 1].x - tmp[i].x) * t;
							tmp[i].y = tmp[i].y + (tmp[i + 1].y - tmp[i].y) * t;
						}
					}
					return tmp[0];
				};

				// split into segments at corners
				std::vector<Vector2> pts2;
				pts2.push_back({ x, y });
				pts2.insert(pts2.end(), points.begin(), points.end());
				
				std::vector<std::vector<Vector2>> segments;
				std::vector<Vector2> current;
				int total = 0;
				current.push_back(pts2[0]);
				
				for (size_t i = 1; i < pts2.size(); i++) {
					if (pts2[i].x == pts2[i - 1].x && pts2[i].y == pts2[i - 1].y) {
						total++;
						corners.push_back({ pts2[i].x, pts2[i].y, (float)total });

						segments.push_back(current);
						current.clear();
					}
					current.push_back(pts2[i]);
				}

				if (!current.empty())
					segments.push_back(current);

				// get points along segments
				bool firstSegment = true;
				int cornerIndex = 0;

				for (auto& seg : segments) {
					const int approxSteps = 32;
					float length = 0.0f;
					float totalIndex = 0.0f;
					Vector2 prev = seg[0];

					for (int i = 1; i <= approxSteps; ++i) {
						float t = (float)i / (float)approxSteps;
						Vector2 p = bezier_point(seg, t);
						float dx = p.x - prev.x;
						float dy = p.y - prev.y;
						length += std::sqrt(dx * dx + dy * dy);
						prev = p;
					}

					int steps = std::max(2, (int)std::ceil(length / slider_resolution));

					int iStart = firstSegment ? 0 : 1;

					float angDeg = 0.0f;
					for (int i = iStart; i <= steps; ++i) {
						float t = (float)i / (float)steps;
						Vector2 p = bezier_point(seg, t);

						angDeg = 0.0f;
						if (!path.empty()) {
							const auto& prevP = path.back();
							float dx = p.x - prevP.x;
							float dy = p.y - prevP.y;
							angDeg = 90 + std::atan2f(dy, dx) * 180.0f / PI;
						}
						totalIndex++;
						path.push_back({ p.x, p.y, angDeg });
					}

					if (cornerIndex < corners.size() && seg.back().x == corners[cornerIndex].x && seg.back().y == corners[cornerIndex].y) {
						corners[cornerIndex].z = totalIndex;
						cornerIndex++;
					}

					firstSegment = false;
				}
				break;
			}
			}

			sliders.push_back(Slider{ (uint16_t)slider_repeat, false, shape, slider_length * playfield_scale, std::move(path), std::move(corners), slider_repeat, {x, y} });
			hit_objects.push_back(HitObjectEntry{ {x, y}, time, end_time, slider_cnt, hit_color_current, hot, combo_idx });
			slider_cnt++;
			break;
		}
		case SPINNER:
			int end_time;
			to_int(sv.substr(start), end_time);
			hit_objects.push_back(HitObjectEntry{ {256 * playfield_scale + playfield_offset_x, 192 * playfield_scale + playfield_offset_y}, time, end_time, spinner_cnt, hit_color_current, hot, combo_idx });
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

	switch (ho.type) {
	case CIRCLE: {
		if (CheckCollisionPointCircle(mouse_pos, ho.pos, circle_radius * 0.94)) {
			if (delta <= hit_window_300) {
				std::cout << "300 with offset " << map_time - ho.time << "!\n";
				hit300s++;
				combo++;
				if (max_combo < combo) max_combo = combo;
				hits.push_back({ ho.pos, draw_hit_time, HIT_300 });
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
			if (CheckCollisionPointCircle(mouse_pos, ho.pos, circle_radius * 0.94)) {
				if (delta <= hit_window_50) {
					std::cout << "Slider head hit! " << map_time - ho.time << "!\n";
					combo++;
					if (max_combo < combo) max_combo = combo;
				}
				else {
					std::cout << "Slider head miss! with offset " << map_time - ho.time << "!\n";
					if (max_combo < combo) max_combo = combo;
					combo = 0;
					hits.push_back({ {s.path.back().x, s.path.back().y}, draw_hit_time, MISS });
				}
			}
		}
		break;
	}
	}
}

void ingame::update() {
	map_time = map_begin_time + GetTime() * 1000.0f * map_speed;
	mouse_pos = { (float)GetMouseX(), (float)GetMouseY() };
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
			if (map_speed != 1.0f) {
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
		if (skippable && (IsKeyPressed(KEY_SPACE) || (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse_pos, { 4*screen_width/5, 4*screen_height/5, screen_width/5, screen_height/5 })))) {
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
		HitObjectEntry ho;
		if (on_object < (int)hit_objects.size()) ho = hit_objects[on_object];
		else break;

		switch (ho.type) {
		case CIRCLE: {
			// check for disappearance

			while (on_object < (int)hit_objects.size() && hit_objects[on_object].end_time < map_time - hit_window_50 * 0.5f) {
				std::cout << "Missed Circle at time " << ho.time << "\n";
				misses++;
				combo = 0;
				hits.push_back({ ho.pos, draw_hit_time, MISS });
				on_object++;
				recalculate_acc();
			}

			break;
		}
		case SLIDER: {
			auto& s = sliders[ho.idx];
			// check for disappearance
			while (on_object < (int)hit_objects.size() && hit_objects[on_object].end_time < map_time) {
				std::cout << "Missed Slider at time " << ho.time << "\n";
				misses++;
				combo = 0;
				hits.push_back({ { s.path.back().x, s.path.back().y }, draw_hit_time, MISS });
				on_object++;
				recalculate_acc();
			}
			
			float path_tick_time = ((float)(ho.end_time - ho.time) / (float)s.repeat_count) / s.path.size();

			int total_length = ho.end_time - ho.time;
			int progress = std::max(std::min((ho.end_time - (int)map_time), ho.end_time - ho.time), 0);

			float progress_norm = (float)progress / (float)total_length;
			float progress_each_tick = 1.0f / s.path.size();
			float on_tick = (1.0f - progress_norm) / progress_each_tick;

			float remainder = on_tick - (int)on_tick;

			if (s.path.size() == 1) { // linear sliders
				float interp_x = (1.0f - remainder) * ho.pos.x + remainder * s.path[0].x;
				float interp_y = (1.0f - remainder) * ho.pos.y + remainder * s.path[0].y;
				s.slider_ball_pos = { interp_x, interp_y };
				break;
			}
			on_tick = std::min(on_tick, (float)s.path.size() - 1);
			float interp_x = (1.0f - remainder) * s.path[(int)on_tick].x + remainder * s.path[std::min((int)on_tick + 1, (int)s.path.size() - 1)].x;
			float interp_y = (1.0f - remainder) * s.path[(int)on_tick].y + remainder * s.path[std::min((int)on_tick + 1, (int)s.path.size() - 1)].y;

			s.slider_ball_pos = { interp_x, interp_y };
			break;
			}		
			
		case SPINNER: {
			// check for disappearance
			while (on_object < (int)hit_objects.size() && hit_objects[on_object].end_time < map_time) {
				std::cout << "Missed Spinner at time " << ho.time << "\n";
				misses++;
				combo = 0;
				hits.push_back({ ho.pos, draw_hit_time, MISS });
				on_object++;
				recalculate_acc();
			}

			break;
		}
		
		}

		
		if (k1_down || k2_down) {
			// implement slider & spinner behavior
			
			
		}

		break;
	}
	
	while (visible_end < (int)hit_objects.size() && hit_objects[visible_end].time - approach_rate_milliseconds <= map_time) {
		visible_end++;
	}

}

void ingame::draw() {
	ClearBackground(BLACK);
	DrawRectangleLines(playfield_offset_x, playfield_offset_y, playfield_scale * 512, playfield_scale * 384, WHITE);

	if (skippable) {
		DrawTexturePro(atlas, tex[28], { screen_width - 1.25f * screen_height / 4, screen_height - screen_height / 4, 1.25f * screen_height / 4, screen_height / 4 }, { 0,0 }, 0.0f, WHITE);
	}

	// --- Draw objects ---
	for (int i = on_object; i < visible_end; i++) {
		const auto& ho = hit_objects[i];
		if (ho.type != SPINNER) continue;

		// spinners always have an approach rate of 400ms from my testing
		unsigned char alpha = (unsigned char)std::clamp(255.0f * ((map_time - (ho.time - 400)) / (400)), 0.0f, 255.0f);
		float x, y;
		x = 256 * playfield_scale + playfield_offset_x;
		y = 192 * playfield_scale + playfield_offset_y;
		DrawCircleV(Vector2{ x, y }, 256 * playfield_scale, { 64, 128, 255, alpha });
		auto radius = (map_time < ho.time) ? 256 * playfield_scale : 240 * playfield_scale * (ho.end_time - map_time) / (ho.end_time - ho.time) + 16;
		DrawCircleLinesV(Vector2{ x, y }, radius, { 255, 255, 255, alpha });
		DrawCircleV(Vector2{ x, y }, 16, { 255, 255, 255, alpha });
	}


	for (int i = visible_end - 1; i >= on_object; i--) {
		auto& ho = hit_objects[i];

		unsigned char alpha = (unsigned char)std::clamp(255.0f * ((map_time - (ho.time - approach_rate_milliseconds)) / (0.666f * approach_rate_milliseconds)), 0.0f, 255.0f);
		float approach_scale = std::max(0.0f, (ho.time - map_time)) / approach_rate_milliseconds * circle_radius * 3 + circle_radius * 0.94;
		Color color = hit_colors[ho.color_idx];
		color.a = alpha;
		Color white_alpha = { 255, 255, 255, alpha };
		Color circle_color = { color.r, color.g, color.b, alpha * 0.8f };
		switch (ho.type) {
		case CIRCLE: {
			DrawTexturePro(atlas, tex[0], { ho.pos.x - approach_scale, ho.pos.y - approach_scale, approach_scale * 2, approach_scale * 2 }, { 0,0 }, 0.0f, color);
			DrawTexturePro(atlas, tex[21], { ho.pos.x - circle_radius, ho.pos.y - circle_radius, circle_radius * 2, circle_radius * 2 }, { 0,0 }, 0.0f, white_alpha);
			DrawTexturePro(atlas, tex[20], { ho.pos.x - circle_radius, ho.pos.y - circle_radius, circle_radius * 2, circle_radius * 2 }, { 0,0 }, 0.0f, circle_color);
			DrawTextEx(aller_r, std::to_string(ho.combo_idx).c_str(), { ho.pos.x - MeasureTextEx(aller_r, std::to_string(ho.combo_idx).c_str(), circle_radius * 1.2f, 0).x / 2.0f, ho.pos.y - circle_radius * 0.6f }, circle_radius * 1.2f, 0, WHITE);
			break;
		}
		case SLIDER: {
			auto& s = sliders[ho.idx];

			DrawTexturePro(atlas, tex[0], { ho.pos.x - approach_scale, ho.pos.y - approach_scale, approach_scale * 2, approach_scale * 2 }, { 0,0 }, 0.0f, color);

			Color body_color = { 80, 80, 80, 0.5f * alpha };
			switch (s.slider_type) {
			case 'L': {
				Rectangle dest = { (ho.pos.x + s.path[0].x) / 2, (ho.pos.y + s.path[0].y) / 2, circle_radius * 1.84f, s.length };
				Vector2 origin = { dest.width / 2.0f, dest.height / 2.0f };

				DrawTexturePro(atlas, tex[61], dest, origin, s.path[0].z, white_alpha);
				break;
			}
			case 'P': {
				for (size_t i = 1; i < s.path.size(); ++i) {
					Rectangle dest = { s.path[i - 1].x, s.path[i - 1].y, circle_radius * 1.84f, 10 };
					Vector2 origin = { dest.width / 2.0f, dest.height / 2.0f };
					DrawTexturePro(atlas, tex[61], dest, origin, s.path[i - 1].z, white_alpha);
				}

				break;
			}
			case 'C':
			case 'B': {
				int indexToCheck = 0;
				for (int i = 0; i < s.corners.size(); i++) {

					indexToCheck += s.corners[i].z;

					float ang1 = ShiftAngleForward(s.path[indexToCheck].z);
					float ang2 = ShiftAngleForward(s.path[indexToCheck - 1].z);

					float inter_x = (s.path[indexToCheck].x + s.path[indexToCheck - 1].x) / 2.f;
					float inter_y = (s.path[indexToCheck].y + s.path[indexToCheck - 1].y) / 2.f;

					if(ang2 < ang1) {
						std::swap(ang1, ang2);
					}
					// TODO : improve unnecessary renders
					for (float j = ang1; j < ang2; j += 5) {
						Rectangle dest = { inter_x, inter_y, circle_radius * 1.84f, 10 };
						Vector2 origin = { dest.width / 2.0f, dest.height / 2.0f };

						DrawTexturePro(atlas, tex[61], dest, origin, j, white_alpha);
					}
				}
				for (size_t i = 1; i < s.path.size(); ++i) {
					Rectangle dest = {s.path[i - 1].x, s.path[i - 1].y, circle_radius * 1.84f, 10 };
					Vector2 origin = { dest.width / 2.0f, dest.height / 2.0f };
					DrawTexturePro(atlas, tex[61], dest, origin, s.path[i - 1].z, white_alpha);
				}

				break;
			}
			default:
				break;
			}

			if (ho.time < map_time)
			DrawCircle(s.slider_ball_pos.x, s.slider_ball_pos.y, circle_radius * 0.92f, YELLOW);

			DrawTexturePro(atlas, tex[21], { ho.pos.x - circle_radius, ho.pos.y - circle_radius, circle_radius * 2, circle_radius * 2 }, { 0,0 }, 0.0f, white_alpha);
			DrawTexturePro(atlas, tex[20], { ho.pos.x - circle_radius, ho.pos.y - circle_radius, circle_radius * 2, circle_radius * 2 }, { 0,0 }, 0.0f, circle_color);

			DrawTexturePro(atlas, tex[21], { s.path.back().x - circle_radius, s.path.back().y - circle_radius, circle_radius * 2, circle_radius * 2 }, { 0,0 }, 0.0f, white_alpha);
			DrawTexturePro(atlas, tex[20], { s.path.back().x - circle_radius, s.path.back().y - circle_radius, circle_radius * 2, circle_radius * 2 }, { 0,0 }, 0.0f, circle_color);
			DrawTextEx(aller_r, std::to_string(ho.combo_idx).c_str(), { ho.pos.x - MeasureTextEx(aller_r, std::to_string(ho.combo_idx).c_str(), circle_radius * 1.2f, 0).x / 2.0f, ho.pos.y - circle_radius * 0.6f }, circle_radius * 1.2f, 0, WHITE);

			

			if (s.repeat_left > 1) {
				if (s.repeat_left > 2) { // both ends
					DrawTextEx(aller_r, "R", { s.path[0].x - MeasureTextEx(aller_r, "r", circle_radius * 1.2f, 0).x / 2.0f, s.path[0].y - circle_radius * 0.6f }, circle_radius * 1.2f, 0, WHITE);
					DrawTextEx(aller_r, "R", { s.path.back().x - MeasureTextEx(aller_r, "r", circle_radius * 1.2f, 0).x / 2.0f, s.path.back().y - circle_radius * 0.6f }, circle_radius * 1.2f, 0, WHITE);
				}
				else {
					if (s.repeat_left % 2 == 1) {
						DrawTextEx(aller_r, "R", { s.path.back().x - MeasureTextEx(aller_r, "r", circle_radius * 1.2f, 0).x / 2.0f, s.path.back().y - circle_radius * 0.6f }, circle_radius * 1.2f, 0, WHITE);
					}
					else {
						DrawTextEx(aller_r, "R", { s.path[0].x - MeasureTextEx(aller_r, "r", circle_radius * 1.2f, 0).x / 2.0f, s.path[0].y - circle_radius * 0.6f }, circle_radius * 1.2f, 0, WHITE);
					}
				}
			}
			break;
		}
		case SPINNER: {
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
			DrawTextEx(aller_r, "300", { h.pos.x - MeasureTextEx(aller_r, "300", 24, 0).x / 2.0f, h.pos.y }, 24, 0, BLUE);
			break;
		case HIT_100:
			DrawTextEx(aller_r, "100", { h.pos.x - MeasureTextEx(aller_r, "100", 24, 0).x / 2.0f, h.pos.y }, 24, 0, GREEN);
			break;
		case HIT_50:
			DrawTextEx(aller_r, "50", { h.pos.x - MeasureTextEx(aller_r, "50", 24, 0).x / 2.0f, h.pos.y }, 24, 0, YELLOW);
			break;
		case MISS:
			DrawTextEx(aller_r, "Miss!", { h.pos.x - MeasureTextEx(aller_r, "Miss!", 24, 0).x / 2.0f, h.pos.y }, 24, 0, RED);
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