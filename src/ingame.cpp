#include "ingame.hpp";
#include <fstream>
#include <filesystem>
#include <iostream>
#include <rlgl.h>

ingame::ingame(file_struct map) {
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

	int circle_cnt = 0;
	int slider_cnt = 0;
	int spinner_cnt = 0;

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
			circles.push_back(Circle{ playfield_offset_x + x * playfield_scale, playfield_offset_y + y * playfield_scale, 0, (uint8_t)(hit_color_current) });
			hit_objects.push_back(HitObjectEntry{ time, time, hot, circle_cnt });
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

			sliders.push_back(Slider{ playfield_offset_x + x * playfield_scale, playfield_offset_y + y * playfield_scale, 0, (uint8_t)(hit_color_current),(uint8_t)slider_repeat, shape, std::move(points)});
			hit_objects.push_back(HitObjectEntry{ time, end_time, hot, slider_cnt });
			slider_cnt++;
			break;
		}
		case SPINNER: 
			int end_time;
			to_int(sv.substr(start), end_time);
			std::cout << "spinner time & end_time: " << time << ", " << end_time << "\n";
			spinners.push_back(Spinner{ end_time });
			hit_objects.push_back(HitObjectEntry{ time, end_time, hot, spinner_cnt });
			spinner_cnt++;
			break;  
		default:
			break;
		}
	}
	map_begin_time = -GetTime() * 1000.0f - 1000.0f;
	
	hit_objects.shrink_to_fit();
	circles.shrink_to_fit();
	sliders.shrink_to_fit();
	spinners.shrink_to_fit();
}

void ingame::update() {
	map_time = map_begin_time + GetTime() * 1000.0f;
	if (!song_init && map_time > 0.0f) {
		PlayMusicStream(music);
		song_init = true;
	}
	//implement skip (5000 ms / bpm)
	
	
	return;
}


void ingame::draw(){
	ClearBackground(BLACK);
	DrawRectangleLines(playfield_offset_x, playfield_offset_y, playfield_scale * 512, playfield_scale * 384, WHITE);

	int visible_end = hit_objects.size();

	while (on_object < (int)hit_objects.size() && hit_objects[on_object].end_time < map_time - hit_window_50 * 0.5f) {
		on_object++;
	}

	while (visible_end < (int)hit_objects.size() && hit_objects[visible_end].time - approach_rate_milliseconds <= map_time + hit_window_50 * 0.5f) {
		visible_end++;
	}

	for (int i = visible_end - 1; i >= on_object; i--) {
		auto& ho = hit_objects[i];

		unsigned char alpha = (unsigned char)std::clamp(255.0f * ((map_time - (ho.time - approach_rate_milliseconds)) / (0.666f * approach_rate_milliseconds)), 0.0f, 255.0f);

		switch (ho.type) {
		case CIRCLE: {
			auto& c = circles[ho.idx];
			float approach_scale = std::max(0.0f, (ho.time - map_time)) / approach_rate_milliseconds * circle_radius * 3 + circle_radius;
			Color color = hit_colors[c.color_idx];
			color.a = alpha;
			DrawCircleLinesV( c.pos, approach_scale, color);
			DrawCircleV(c.pos, circle_radius , color);
			break;
		}
		case SLIDER: {
			auto& s = sliders[ho.idx];
			float approach_scale = std::max(0.0f, (ho.time - map_time)) / approach_rate_milliseconds * circle_radius * 3 + circle_radius;
			Color color = hit_colors[s.color_idx];
			color.a = alpha;
			DrawCircleLinesV(s.pos, approach_scale, color);
			
			Color body_color = { 80, 80, 80, 0.5f*alpha };
			switch (s.slider_type) { // TODO: Learn math bro...
			case 'L': {
				DrawLineEx(s.pos, s.points[0], circle_radius*2, body_color);
				break;
			}
			case 'P': {
				//DrawCircleSector(s.pos, circle_radius, 0.0f, 120.0f, 36, body_color);
				break;
			}
			case 'C':
			case 'B': {
				// DrawSplineBezierQuadratic(s.points.data(), (int)s.points.size(), circle_radius*2, body_color);
				break;
			}
			default:
				break;
			}
			DrawCircleV(s.pos, circle_radius, color);
			DrawCircleV(s.points[s.points.size() - 1], circle_radius, color);

			break;
		}
		case SPINNER: {
			alpha = (unsigned char)std::clamp(255.0f * ((map_time - (ho.time - approach_rate_milliseconds)) / (approach_rate_milliseconds)), 0.0f, 255.0f);
			auto& s = spinners[ho.idx];
			float x, y;
			x = 256 * playfield_scale + playfield_offset_x;
			y = 192 * playfield_scale + playfield_offset_y;
			DrawCircleV(Vector2{ x, y }, 256 * playfield_scale, { 64, 128, 255, alpha });
			DrawCircleLinesV(Vector2{ x, y }, 192 * (ho.end_time - map_time) / (ho.end_time - ho.time) + 16, { 255, 255, 255, alpha });
			DrawCircleV(Vector2{ x, y }, 16, { 255, 255, 255, alpha });
			break;
		}
		default:
			break;
		}
	}
	return;
}