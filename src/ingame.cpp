#include "ingame.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <rlgl.h>
#include "result_screen.hpp"
#include "globals.hpp"

ingame::ingame(file_struct map) {
	map_info = map;
	hit_window_300 = (int)(80.0f - 6.0f * map_info.od);
	hit_window_100 = (int)(140.0f - 8.0f * map_info.od);
	hit_window_50 = (int)(200.0f - 10.0f * map_info.od);
	approach_rate_milliseconds = (map_info.ar < 5.0f) ? int(1800 - 120 * map_info.ar) : int(1200 - 150 * (map_info.ar - 5));

	spinner_rotation_ratio = (map_info.od < 5.0f) ? 5.0f - 2.0f * (5.0f - map_info.od) / 5.0f : 5.0f + 2.5f * (map_info.od - 5.0f) / 5.0f;

	circle_radius = playfield_scale * (54.4f - (4.48f * map_info.cs));

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
	uint32_t timing_point_idx = 0;
	uint32_t combo_idx = 0;

	float ms_per_tick = timing_points[timing_point_idx].ms_beat / map_info.slider_tickrate;
	tick_draw_delay = std::min(40.0f, ms_per_tick * 0.5f);

	while (std::getline(infile, line)) {
		static Vector2 last_object_pos = { -1000.0f, -1000.0f };
		static bool stack_leniency_downward = false;

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
		float x, y;

		switch (hot) {
		case CIRCLE: {
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
			// find correct timing point for this slider
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
			end_time = time + (int)(slider_repeat * slider_length * timing_points[timing_point_idx].ms_beat / (100 * timing_points[timing_point_idx].slider_velocity));
			
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

			// slider path calculations

			std::vector<Vector3> path;
			std::vector<Vector3> corners;
			std::vector<Vector4> slider_ticks;

			switch (shape) {
			case 'L': {
				// linear slider
				process_as_linear_slider:
				auto angle = atan2(points.back().y - y, points.back().x - x);
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

				float start = std::atan2(p0.y - center.y, p0.x - center.x);
				float mid = ShiftRadiansForward(std::atan2(p1.y - center.y, p1.x - center.x), start);
				float end = ShiftRadiansForward(std::atan2(p2.y - center.y, p2.x - center.x), start);

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
					p.x = center.x + std::cos(ang) * radius;
					p.y = center.y + std::sin(ang) * radius;
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

				if (points.size() == 1) {
					shape = 'L';
					goto process_as_linear_slider;
				}

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
				uint32_t cornerIndex = 0;

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
							angDeg = 90 + std::atan2(dy, dx) * 180.0f / PI;
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

			// depending on slider length & tickrate, generate ticks
			// interpolate along path 
			float total_length = float(end_time - time) / (float)slider_repeat;
		
			int tick_amount = (int)(total_length / ms_per_tick);
			if (std::fmodf(total_length, ms_per_tick) <= 3.0f) tick_amount--;

			float progress_each_tick = 1.0f / path.size();
			for (int i = 1; i <= tick_amount; ++i) {
				float tick_time = i * ms_per_tick;
				float progress_norm = tick_time / total_length;

				float on_tick = progress_norm / progress_each_tick;

				float remainder = on_tick - (int)on_tick;

				if (path.size() == 1) { // linear sliders
					float interp_x = (1.0f - remainder) * x + remainder * path[0].x;
					float interp_y = (1.0f - remainder) * y + remainder * path[0].y;
					slider_ticks.push_back({ interp_x, interp_y, time + tick_time, 0});
					continue;
				}
				on_tick = std::min(on_tick, (float)path.size() - 1);
				float interp_x = (1.0f - remainder) * path[(int)on_tick].x + remainder * path[std::min((int)on_tick + 1, (int)path.size() - 1)].x;
				float interp_y = (1.0f - remainder) * path[(int)on_tick].y + remainder * path[std::min((int)on_tick + 1, (int)path.size() - 1)].y;
				slider_ticks.push_back({ interp_x, interp_y, time + tick_time, 0 });
			}

			int n = slider_ticks.size();
			for (int i = 1; i < slider_repeat; ++i) {
				// push reverse arrows as slider ticks
				
				if(i % 2 == 1) {
					auto& t = path.back();
					slider_ticks.push_back(Vector4{ t.x, t.y, time + i * (float)total_length, 2 });
				}
				else {
					slider_ticks.push_back(Vector4{ x, y, time + i * (float)total_length, 2 });
				}

				int last_repeat_time = (int)(i * total_length);
				for (int j = 0; j < n; j++) {
					auto& t = slider_ticks[j];
					if(i % 2 == 1)
						slider_ticks.push_back(Vector4{ t.x, t.y, 2.0f * time + (i + 1) * (float)total_length - t.z, 1 });
					else slider_ticks.push_back(Vector4{ t.x, t.y, t.z + last_repeat_time, 1 });
				}
			}

			uint32_t slider_end_check_time = (end_time - time) / 2;
			slider_end_check_time = std::min(36U, slider_end_check_time);

			uint32_t total_hits = (uint32_t)slider_ticks.size() + 2; // including head & tail

			sliders.push_back(Slider{ slider_repeat, false, false, false, false, slider_length * playfield_scale, slider_repeat, {x, y}, std::move(path), std::move(corners), std::move(slider_ticks), 0, slider_end_check_time, total_hits, 0, false, shape});
			hit_objects.push_back(HitObjectEntry{ {x, y}, time, end_time, slider_cnt, hit_color_current, hot, combo_idx });
			slider_cnt++;

			break;
		}
		case SPINNER: {
			int end_time;
			to_int(sv.substr(start), end_time);

			int length = end_time - time;
			int rotation_req = (int)((float)length / 1000.0f * spinner_rotation_ratio);
			double max_acceleration = 0.00008 + std::max(0.0, (5000.0 - length) / 2000000.0);

			spinners.push_back(Spinner{ rotation_req, 0.0f, max_acceleration, 0.0f, 0.0 });
			hit_objects.push_back(HitObjectEntry{ {256 * playfield_scale + playfield_offset_x, 192 * playfield_scale + playfield_offset_y}, time, end_time, spinner_cnt, hit_color_current, hot, combo_idx });
			spinner_cnt++;

			break;
		}
			
		default:
			break;
		}

		// stack leniency

		index = (int)hit_objects.size() - 1;
		static bool under_slider = false;
		static Vector2 to_compare = { 0, 0 };
		static int stack_counter = 1;

		while( index > 0 ) {
			auto& cur_obj = hit_objects[index];
			auto& prev_obj = hit_objects[index - 1];
			if (cur_obj.time - prev_obj.end_time > (int)(approach_rate_milliseconds * map_info.stack_leniency)) {
				break;
			}

			switch (prev_obj.type) {
			case CIRCLE:
				begin_circle_check:
				if (under_slider) {
					if (roughly_equal(to_compare, cur_obj.pos)) {
						cur_obj.pos.x += 3.5f * playfield_scale * stack_counter;
						cur_obj.pos.y += 3.5f * playfield_scale * stack_counter;
						stack_counter++;
						goto end_loop;
					}
					else {
						under_slider = false;
						stack_counter = 1;
						goto begin_circle_check;
					}
				}
				else if (roughly_equal(cur_obj.pos, prev_obj.pos)) {
					prev_obj.pos.x -= 3.5f * playfield_scale;
					prev_obj.pos.y -= 3.5f * playfield_scale;
				}
				else goto end_loop;
			break;
			case SLIDER:
				// check both beginning & end
				if (roughly_equal(cur_obj.pos, prev_obj.pos)) {
					prev_obj.pos.x -= 3.5f * playfield_scale;
					prev_obj.pos.y -= 3.5f * playfield_scale;
					for( auto& p : sliders[prev_obj.idx].path ) {
						p.x -= 3.5f * playfield_scale;
						p.y -= 3.5f * playfield_scale;
					}
				}
				else if (roughly_equal({ sliders[prev_obj.idx].path.back().x, sliders[prev_obj.idx].path.back().y }, cur_obj.pos)) {
					// reverse everythin' in this case
					// they all have to be *under* the slider...
					// TODO : some maps dont work? debug (oyasumi extra/expert i forgor which and super girl hard diff)
					under_slider = true;
					to_compare = { cur_obj.pos.x, cur_obj.pos.y };
					cur_obj.pos.x += 3.5f * playfield_scale;
					cur_obj.pos.y += 3.5f * playfield_scale;
					stack_counter++;
					goto end_loop;
				}
				else goto end_loop;
			break;
			}

			index--;
		}
	end_loop:;
	}
	map_begin_time = -(float)GetTime() * 1000.0f * map_speed - 1000.0f;

	hit_objects.shrink_to_fit();
	sliders.shrink_to_fit();

	map_first_object_time = hit_objects[0].time;

	// calculate max TOTAL COMBO
	int total_max_combo = 0;
	for (auto& ho : hit_objects) {
		switch (ho.type) {
		case CIRCLE:
			total_max_combo++;
			break;
		case SLIDER:
			total_max_combo += 2 + (int)(sliders[ho.idx].slider_ticks.size());
			break;
		case SPINNER:
			total_max_combo += 1;
			break;
		}
	}

	std::cout << "Map loaded: " << circle_cnt << " circles, " << slider_cnt << " sliders, " << spinner_cnt << " spinners.\n" << "Max combo: " << total_max_combo << "\n";

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

void ingame::check_hit(bool notelock_check) {
	auto& ho = hit_objects[on_object];
	float delta = std::abs(map_time - ho.time);
	if (delta > 360) return; // way too early
	if (notelock_check && delta > hit_window_300) return; // too early for hit
	switch (ho.type) {
	case CIRCLE: {
		if (CheckCollisionPointCircle(mouse_pos, ho.pos, circle_radius * 0.94f)) {
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
		if (!s.head_hit_checked) {
			delta = std::abs(map_time - ho.time);
			if (delta > 360) return;
			if (CheckCollisionPointCircle(mouse_pos, ho.pos, circle_radius * 0.94f)) {
				s.head_hit_checked = true;
				if (delta <= hit_window_50) {
					std::cout << "Slider head hit! " << map_time - ho.time << "!\n";
					s.head_hit = true;
					combo++;
					if (max_combo < combo) max_combo = combo;
				}
				else {
					std::cout << "Slider head miss! with offset " << map_time - ho.time << "!\n";
					if (max_combo < combo) max_combo = combo;
					combo = 0;
					s.head_hit_checked = true;
				}
			}
		}
		break;
	}
	}
}

void ingame::update() {
	map_time = map_begin_time + (float)GetTime() * 1000.0f * map_speed;
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
		return;
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

			map_begin_time = skip_target_time - (float)GetTime() * 1000.0f * map_speed;

			skippable = false;
			song_init = 2;
		}
		break;
	case 2: // map ongoing
		int prev_on_object = on_object;
		int check_cnt = 0;
	update_object_jmp: // do all this for the 2 objects to prevent notelock
		if (IsKeyPressed(key_1)) {
			key1_down = true;
			check_hit(bool(check_cnt));
		}
		if (IsKeyPressed(key_2)) {
			key2_down = true;
			check_hit(bool(check_cnt));
		}
		if (IsKeyReleased(key_1)) {
			key1_down = false;
		}
		if (IsKeyReleased(key_2)) {
			key2_down = false;
		}
		
		HitObjectEntry ho;
		if (on_object < (int)hit_objects.size()) ho = hit_objects[on_object];
		else break;

		switch (ho.type) {
		case CIRCLE: {
			// check for disappearance

			while (on_object < (int)hit_objects.size() && hit_objects[on_object].end_time < map_time - hit_window_50) {
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

			if(!s.head_hit_checked) {
				if (ho.time + hit_window_50 < map_time) {
					std::cout << "Slider head miss at time " << ho.time << "\n";
					s.head_hit_checked = true;
					combo = 0;
				}
			}

			if (key1_down || key2_down) { // check if slider is being tracked rn
				if (s.tracked) {
					s.tracked = CheckCollisionPointCircle(mouse_pos, { s.slider_ball_pos.x, s.slider_ball_pos.y }, circle_radius * 0.94f * 2.4f);
				}
				else {
					s.tracked = CheckCollisionPointCircle(mouse_pos, { s.slider_ball_pos.x, s.slider_ball_pos.y, }, circle_radius * 0.94f);
				}
			} else {
				s.tracked = false;
			}

			// check for slider tick & reverse arrow hits
			if (s.on_slider_tick < s.slider_ticks.size()) {
				if (s.slider_ticks[s.on_slider_tick].z <= map_time) { // time to check
					if (key1_down || key2_down) {
						bool hit_tick = CheckCollisionPointCircle(mouse_pos, { s.slider_ticks[s.on_slider_tick].x, s.slider_ticks[s.on_slider_tick].y }, circle_radius * 0.94f * slider_body_hit_radius);
						if (hit_tick) {
							std::cout << "Slider tick / reverse arrow hit at time " << s.slider_ticks[s.on_slider_tick].z << "\n";
							s.successful_hits++;
							combo++;
							if (max_combo < combo) max_combo = combo;
							goto slider_tick_check_continue;
						}
					}
					std::cout << "Slider tick miss at time " << s.slider_ticks[s.on_slider_tick].z << "\n";
					combo = 0;
					slider_tick_check_continue:
					s.on_slider_tick++;
				}
			}

			// check end hit
			if (!s.tail_hit_checked && ho.end_time - s.slider_end_check_time < map_time) {
				s.tail_hit_checked = true;
				if (key1_down || key2_down) {
					if(s.repeat_count % 2 == 0) // even repeat count, end is at beginning
						s.tail_hit = CheckCollisionPointCircle(mouse_pos, { ho.pos.x, ho.pos.y }, circle_radius * 0.94f * slider_body_hit_radius);
					else // odd repeat count, end is at end
					s.tail_hit = CheckCollisionPointCircle(mouse_pos, { s.path.back().x, s.path.back().y }, circle_radius * 0.94f * slider_body_hit_radius);
					std::cout << "Slider end hit at time " << ho.end_time - s.slider_end_check_time << "\n";
					combo++;
				}
			}

			// check for disappearance
			while (on_object < (int)hit_objects.size() && hit_objects[on_object].end_time < map_time) {
				// check end hit
				
				uint32_t successful_hits = s.successful_hits;
				if (s.head_hit) successful_hits++;
				if (s.tail_hit) successful_hits++;

				Vector2 end_pos;
				if (s.repeat_count % 2 == 0) // even repeat count, end is at beginning
					end_pos = ho.pos;
				else // odd repeat count, end is at end
					end_pos = { s.path.back().x, s.path.back().y };

				if (successful_hits >= s.total_hits) {
					std::cout << "Slider fully hit at time " << ho.time << "\n";
					hit300s++;
					if (max_combo < combo) max_combo = combo;
					hits.push_back({ end_pos, draw_hit_time, HIT_300 });
				}
				else if (successful_hits * 2.0f >= s.total_hits) {
					std::cout << "Slider 100 at time " << ho.time << "with " << successful_hits << " hits out of " << s.total_hits << "\n";
					hit100s++;
					hits.push_back({ end_pos, draw_hit_time, HIT_100 });
				} else if (successful_hits > 0) {
					std::cout << "Slider 50 at time " << ho.time << "with " << successful_hits << " hits out of " << s.total_hits << "\n";
					hit50s++;
					hits.push_back({ end_pos, draw_hit_time, HIT_50 });
				}
				else {
					std::cout << "Slider miss at time " << ho.time << "with " << successful_hits << " hits out of " << s.total_hits << "\n";
					misses++;
					combo = 0;
					hits.push_back({ end_pos, draw_hit_time, MISS });
				}


				on_object++;
				recalculate_acc();
			}
			
			// update slider ball position
			float path_tick_time = ((float)(ho.end_time - ho.time) / (float)s.repeat_count) / s.path.size();

			float total_length = (float)(ho.end_time - ho.time);
			float progress = (map_time - (float)ho.time) / total_length;
			progress = std::clamp(progress, 0.0f, 1.0f);

			float progress_each_tick = 1.0f / s.path.size();

			int repeat_index = (int)(progress * s.repeat_count);
			repeat_index = std::clamp(repeat_index, 0, s.repeat_count - 1);

			float repeat_progress = (progress * s.repeat_count) - repeat_index;
			repeat_progress = std::clamp(repeat_progress, 0.0f, 1.0f);

			if(repeat_index % 2 == 1) {
				repeat_progress = 1.0f - repeat_progress;
			}

			float on_tick = repeat_progress / progress_each_tick;
			float remainder = on_tick - (int)on_tick;

			if (s.repeat_left != s.repeat_count - repeat_index) {
				std::cout << "Slider reverse arrow at time " << map_time << "\n";
				s.repeat_left = s.repeat_count - repeat_index;
			}
			

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
			if (check_cnt == 1) break;

			auto& sp = spinners[ho.idx];
			double decay = std::powf(0.9f, GetFrameTime() * 1000.0 * 16.6667);
			sp.rpm = sp.rpm * decay + (1.0 - decay) * std::abs(sp.velocity) * 1000 / PI * 2 * 60.0f;

			double max_accel_this_frame = sp.max_acceleration * GetFrameTime() * 1000.0f;

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

		if(check_cnt == 0) {
			check_cnt++;
			if (prev_on_object == on_object) {
				if (ho.type == SLIDER) {
					auto& s = sliders[ho.idx];
					if (s.head_hit_checked) {
						// no need to check for other Shit.
						break;
					}
				}

				on_object++;
				goto update_object_jmp;
			}
		}
		else {
			// TODO : bugs here if the last object was a slider, rework this
			if (on_object == prev_on_object + 1) {
				if (ho.type == SLIDER) {
					auto& s = sliders[ho.idx];
					if (s.head_hit_checked) {
						std::cout << "prevented notelock on slider head/tail, on_object was " << prev_on_object << " now " << on_object << "\n";
						// miss old object
						auto& ho = hit_objects[prev_on_object];
						std::cout << "Missed Object at time " << ho.time << "\n";
						misses++;
						combo = 1;
						hits.push_back({ ho.pos, draw_hit_time, MISS });
						recalculate_acc();
						break;
					}
				}
				on_object--;
			}
				
			else { // we hit something
				std::cout << "prevented notelock, on_object was " << prev_on_object << " now " << on_object << "\n";
				// miss old object
				auto& ho = hit_objects[prev_on_object];
				std::cout << "Missed Object at time " << ho.time << "\n";
				misses++;
				combo = 1;
				hits.push_back({ ho.pos, draw_hit_time, MISS });
				recalculate_acc();
			}
		}

		break;
	}
	
	while (visible_end < (int)hit_objects.size() && hit_objects[visible_end].time - approach_rate_milliseconds <= map_time) {
		visible_end++;
	}
}

void ingame::draw() {
	ClearBackground(BLACK);
	DrawRectangleLinesF(playfield_offset_x, playfield_offset_y, playfield_scale * 512.0f, playfield_scale * 384.0f, WHITE);

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
		float approach_scale = std::max(0.0f, (ho.time - map_time)) / approach_rate_milliseconds * circle_radius * 3.0f + circle_radius * 0.94f;
		Color color = hit_colors[ho.color_idx];
		color.a = alpha;
		Color white_alpha = { 255, 255, 255, alpha };
		Color circle_color = { color.r, color.g, color.b, (unsigned char)(alpha * 0.8f) };

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
			
			// todo : optimize by having a circle texture instead of doing this
			for (float j = 0; j < 180; j += 10) {
				Rectangle dest = { ho.pos.x, ho.pos.y, circle_radius * 1.84f, 10 };
				Vector2 origin = { dest.width / 2.0f, dest.height / 2.0f };

				DrawTexturePro(atlas, tex[61], dest, origin, j, white_alpha);
			}

			if(!settings_sliderend_rendering) {
				// todo : optimize by having a circle texture instead of doing this
				for (float j = s.path.back().z - 180; j < s.path.back().z; j += 10) {
					Rectangle dest = { s.path.back().x, s.path.back().y, circle_radius * 1.84f, 10 };
					Vector2 origin = { dest.width / 2.0f, dest.height / 2.0f };

					DrawTexturePro(atlas, tex[61], dest, origin, j, white_alpha);
				}
			}

			Color body_color = { 80, 80, 80, (unsigned char)(alpha * 0.5f) };
			switch (s.slider_type) {
			case 'L': {
				Rectangle dest = { (ho.pos.x + s.path[0].x) / 2, (ho.pos.y + s.path[0].y) / 2, circle_radius * 1.84f, s.length };
				Vector2 origin = { dest.width / 2.0f, dest.height / 2.0f };

				DrawTexturePro(atlas, tex[61], dest, origin, s.path[0].z, white_alpha);
				break;
			}
			case 'P': {
				for (size_t i = 1; i < s.path.size(); ++i) {
					Rectangle dest = { s.path[i - 1].x, s.path[i - 1].y, circle_radius * 1.84f, slider_draw_resolution };
					Vector2 origin = { dest.width / 2.0f, dest.height / 2.0f };
					DrawTexturePro(atlas, tex[61], dest, origin, s.path[i - 1].z, white_alpha);
				}

				break;
			}
			case 'C':
			case 'B': {
				uint32_t indexToCheck = 0;
				for (size_t i = 0; i < s.corners.size(); i++) {

					indexToCheck += (int)s.corners[i].z;

					float ang1 = ShiftAngleForward(s.path[indexToCheck].z);
					float ang2 = ShiftAngleForward(s.path[indexToCheck - 1].z);

					float inter_x = (s.path[indexToCheck].x + s.path[indexToCheck - 1].x) / 2.f;
					float inter_y = (s.path[indexToCheck].y + s.path[indexToCheck - 1].y) / 2.f;

					if(ang2 < ang1) {
						std::swap(ang1, ang2);
					}
					for (float j = ang1; j < ang2; j += 5) {
						Rectangle dest = { inter_x, inter_y, circle_radius * 1.84f, slider_draw_resolution };
						Vector2 origin = { dest.width / 2.0f, dest.height / 2.0f };

						DrawTexturePro(atlas, tex[61], dest, origin, j, white_alpha);
					}
				}
				for (size_t i = 1; i < s.path.size(); ++i) {
					Rectangle dest = {s.path[i - 1].x, s.path[i - 1].y, circle_radius * 1.84f, slider_draw_resolution };
					Vector2 origin = { dest.width / 2.0f, dest.height / 2.0f };
					DrawTexturePro(atlas, tex[61], dest, origin, s.path[i - 1].z, white_alpha);
				}

				break;
			}
			default:
				break;
			}

			if (ho.time < map_time) // draw slider ball
			DrawCircleV({ s.slider_ball_pos }, circle_radius * 0.92f, YELLOW);

			
			if (!s.head_hit_checked) {
				DrawTexturePro(atlas, tex[21], { ho.pos.x - circle_radius, ho.pos.y - circle_radius, circle_radius * 2.0f, circle_radius * 2.0f }, { 0,0 }, 0.0f, white_alpha);
				DrawTexturePro(atlas, tex[20], { ho.pos.x - circle_radius, ho.pos.y - circle_radius, circle_radius * 2.0f, circle_radius * 2.0f }, { 0,0 }, 0.0f, circle_color);
				DrawTextEx(aller_r, std::to_string(ho.combo_idx).c_str(), { ho.pos.x - MeasureTextEx(aller_r, std::to_string(ho.combo_idx).c_str(), circle_radius * 1.2f, 0).x / 2.0f, ho.pos.y - circle_radius * 0.6f }, circle_radius * 1.2f, 0, WHITE); // TODO : revamp text rendering for sliders
			}

			if (settings_sliderend_rendering) {
				DrawTexturePro(atlas, tex[21], { s.path.back().x - circle_radius, s.path.back().y - circle_radius, circle_radius * 2.0f, circle_radius * 2.0f }, { 0,0 }, 0.0f, white_alpha);
				DrawTexturePro(atlas, tex[20], { s.path.back().x - circle_radius, s.path.back().y - circle_radius, circle_radius * 2.0f, circle_radius * 2.0f }, { 0,0 }, 0.0f, circle_color);
			}
					
			// draw reverse arrows
			if (s.repeat_left > 1) {
				if (s.repeat_left > 2) {
					// draw both
					Rectangle dest = { ho.pos.x, ho.pos.y, circle_radius * 2.0f, circle_radius * 2.0f };
					Vector2 origin = { dest.width / 2.0f, dest.height / 2.0f };
					if (s.head_hit_checked) DrawTexturePro(atlas, tex[43], dest, origin, 270.0f + s.path.back().z, white_alpha);
					dest = { s.path.back().x, s.path.back().y, circle_radius * 2.0f, circle_radius * 2.0f };

					DrawTexturePro(atlas, tex[43], dest, origin, 90.0f + s.path[0].z, white_alpha);
				}
				else {
					if (s.repeat_count % 2 - s.repeat_left % 2 == 0) {
						// draw at end
							Rectangle dest = { s.path.back().x, s.path.back().y, circle_radius * 2.0f, circle_radius * 2.0f };
							Vector2 origin = { dest.width / 2.0f, dest.height / 2.0f };
							DrawTexturePro(atlas, tex[43], dest, origin, 90.0f + s.path.back().z, white_alpha);
					}
					else {
						// draw at start
						if (s.head_hit_checked) {
							Rectangle dest = { ho.pos.x, ho.pos.y, circle_radius * 2.0f, circle_radius * 2.0f };
							Vector2 origin = { dest.width / 2.0f, dest.height / 2.0f };
							DrawTexturePro(atlas, tex[43], dest, origin, 270.0f + s.path[0].z, white_alpha);
						}
					}
				}
			}

			// draw slider ticks
			int ind_st = 0;
			for (auto& st : s.slider_ticks) {
				if (st.w > 0.0f) continue; // already hit or reverse arrow
				
				if (st.z < map_time + approach_rate_milliseconds * 0.75f + ind_st * tick_draw_delay) {
					DrawCircleV({ st.x, st.y }, circle_radius * 0.25f, white_alpha);
				}
				else {
					break;
				}
				ind_st++;
			}

			// draw ball mouse radius
			if (ho.time <= map_time) {
				if (s.tracked) {
					DrawTexturePro(atlas, tex[0], { s.slider_ball_pos.x - circle_radius * slider_body_hit_radius, s.slider_ball_pos.y - circle_radius * slider_body_hit_radius, circle_radius * slider_body_hit_radius * 2.0f, circle_radius * slider_body_hit_radius * 2.0f }, { 0,0 }, 0.0f, YELLOW);
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
	
	// --- Draw UI ---
	DrawTextExScaled(aller_r, (std::to_string(combo) + "x").c_str(), { 0, screen_height - screen_height / 16.0f }, 48, 0, WHITE);
}