#include "settings.hpp"
#include <algorithm>

void settings::update_screen_resolution(int width, int height) {
	SetWindowSize(width, height);

	screen_width = (float)width;
	screen_height = (float)height;

	screen_width_ratio = screen_width / 1024.0f;
	screen_height_ratio = screen_height / 768.0f;

	playfield_scale = screen_height / 480.0f;
	playfield_offset_x = (screen_width - 512.0f * playfield_scale) / 2.0f;
	playfield_offset_y = (screen_height - 384.0f * playfield_scale) / 2.0f + 8.0f * playfield_scale;

	screen_scale = std::min(screen_width_ratio, screen_height_ratio);
	selected_resolution = { screen_width, screen_height };
	GLFWmonitor* mon = glfwGetPrimaryMonitor();
	auto mode = glfwGetVideoMode(mon);
	screen_refresh_rate = mode->refreshRate;
	init();
}

void settings::init() {
	// Get resolutions

	if (!initialized) {
		GLFWmonitor* mon = glfwGetPrimaryMonitor();
		auto mode = glfwGetVideoMode(mon);
		screen_refresh_rate = mode->refreshRate;

		int count;
		const GLFWvidmode* modes = glfwGetVideoModes(mon, &count);

		for (int i = 0; i < count; i++)
		{
			Vector2 res = { (float)modes[i].width, (float)modes[i].height };

			bool exists = false;
			for (auto& r : available_resolutions)
				if (r.x == res.x && r.y == res.y)
					exists = true;

			if (!exists)
				available_resolutions.push_back(res);
		}
		initialized = true;
	}

	// Initialize switchs

	const float sh = screen_height_ratio;
	switches.clear();
	switches_func.clear();
	sliders.clear();
	sliders_func.clear();
	textfields.clear();
	lists.clear();

	switches.push_back({ {48 * sh, 48 * sh}, {}, 16 * sh,
						settings_render_ingame_ui,
						"Render ingame UI"
		});
	switches.push_back({ {48 * sh, 96 * sh}, {}, 16 * sh,
						settings_sliderend_rendering,
						"Render slider ends"
		});
	switches.push_back({ {48 * sh, 144 * sh}, {}, 16 * sh,
						settings_render_300s,
						"Display 300 hitresults"
		});
	switches.push_back({ {48 * sh, 192 * sh}, {}, 16 * sh,
						settings_render_fps_ms,
						"Display FPS / frametime"
		});
	switches.push_back({ {48 * sh, 240 * sh}, {}, 16 * sh,
						settings_render_play_area,
						"Display play area rectangle"
		});
	switches.push_back({ {48 * sh, 288 * sh}, {}, 16 * sh,
						settings_display_ur_bar,
						"Display UR bar"
		});
	switches.push_back({ {48 * sh, 336 * sh}, {}, 16 * sh,
						settings_ignore_map_colors,
						"Ignore map colors"
		});
	switches.push_back({ {48 * sh, 384 * sh}, {}, 16 * sh,
						settings_display_background_ingame,
						"Display map background ingame"
		});
	switches.push_back({ {48 * sh, 432 * sh}, {}, 16 * sh,
						settings_render_key_overlay,
						"Display key overlay"
		});
	switches.push_back({ {48 * sh, 480 * sh}, {}, 16 * sh,
						settings_ingame_mouse_buttons,
						"Enable mouse buttons ingame"
		});

	switches_func.push_back(gui_switch_func{ {{48 * sh, 528 * sh}, {}, 16 * sh,
						settings_raw_input,
						"Raw input"},
						*callback_raw_input
		});

	
	sliders.push_back({ {(400) * sh, 48 * sh, 160 * sh, 32 * sh}, 0.0f, 0.2f, 5.0f, 0.01f,
						ur_bar_size, false,
						"UR bar size", "x"
		});

	sliders_func.push_back(gui_slider_func{{ {(400) * sh, 112 * sh, 160 * sh, 32 * sh}, 0.0f, 0.2f, 5.0f, 0.01f,
						settings_mouse_sens, false,
						"Sensitivity", "x" },
						*callback_update_sensitivity
		});

	sliders.push_back({ {(400) * sh, 176 * sh, 160 * sh, 32 * sh}, 0.0f, 0.2f, 4.0f, 0.01f,
						settings_cursor_scale, false,
						"Cursor size", "x"
		});
	sliders_func.push_back(gui_slider_func{ {{(400) * sh, 256 * sh, 160 * sh, 32 * sh}, 0.0f, 0.0f, 100.0f, 1.0f,
						settings_volume_master, false,
						"Master volume", "%" },
						*callback_update_volume_master 
		});
	sliders_func.push_back(gui_slider_func{ {{(400) * sh, 328 * sh, 160 * sh, 32 * sh}, 0.0f, 0.0f, 100.0f, 1.0f,
						settings_volume_music, false,
						"Music volume", "%" },
						*callback_update_volume_music
		});
	sliders_func.push_back(gui_slider_func{ {{(400) * sh, 400 * sh, 160 * sh, 32 * sh}, 0.0f, 0.0f, 100.0f, 1.0f,
						settings_volume_sfx, false,
						"Sound effects volume", "%" },
						*callback_update_volume_sound
		});

	std::vector<std::string> resolutions_text;
	for (size_t i = 0; i < available_resolutions.size(); i++) {
		auto& res = available_resolutions[i];
		resolutions_text.push_back(std::to_string((int)res.x) + "x" + std::to_string((int)res.y));
	}

	float res_menu_height = (float)resolutions_text.size() * 32.0f * sh;
	lists.push_back({ {784 * sh, 48 * sh, 192 * sh, res_menu_height }, 0.0f, 0, 32.0f * sh, false,
						*callback_update_resolution,
						"Resolutions", resolutions_text 
		});

	for (auto& b : switches) {
		b.center = { b.pos.x + b.size / 2.0f, b.pos.y + b.size / 2.0f };
	}
	for (auto& b : switches_func) {
		b.center = { b.pos.x + b.size / 2.0f, b.pos.y + b.size / 2.0f };
	}
	for (auto& s : sliders) {
		s.percentage = s.value / s.max_value;
	}
	for (auto& s : sliders_func) {
		s.percentage = s.value / s.max_value;
	}
}

void settings::go_back() {
	selected_element = nullptr;
	game_state = MAIN_MENU;
}

void settings::update() {
	auto mouse_pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
	Vector2 mouse_location = GetMousePosition();

	DrawRectangleGradientV(0, 0, (int)screen_width, (int)screen_height, BLUE, DARKBLUE);
	if (IsKeyPressed(KEY_B)) {
		go_back();
	}

	if (m1_pressed) selected_element = nullptr;
	
	for (auto& b : switches) {
		update_switch(b);
		draw_switch(b);
	}
	for (auto& b : switches_func) {
		update_switch_func(b);
		draw_switch(b);
	}
	for (auto& s : sliders) {
		update_slider(s);
		draw_slider(s);
	}
	for (auto& s : sliders_func) {
		update_slider_func(s);
		draw_slider(s);
	}
	for (auto& l : lists) {
		update_dropdown_list(l);
		draw_dropdown_list(l);
	}

}

void settings::callback_raw_input()
{
	if (settings_raw_input) {
		EnableCursor();
		SetMousePosition(cursor.x, cursor.y);
		HideCursor();
		settings_raw_input = false;
		settings_mouse_sens = 1.0f;
		for (auto& s : sliders_func) {
			update_slider_func(s);
			s.percentage = s.value / s.max_value;
			draw_slider(s);
		}
	}
	else {
		DisableCursor();
		settings_raw_input = true;
	}
}

void settings::callback_update_sensitivity(float) {
	if (!settings_raw_input) {
		callback_raw_input();
		notices.push_back({ "Raw input has been toggled on, as it's required for sensitivity settings to work", 7.5f });
	}
}

void settings::update_switch(gui_switch& b)
{ // && cursor.x >= b.pos.x && cursor.x <= b.pos.x + b.size && cursor.y >= b.pos.y && cursor.y <= b.pos.x + b.size
	if (m1_pressed && CheckCollisionPointCircle(cursor, b.center, b.size)) {
		b.option = !b.option;
		play_sound_effect("normal-hitnormal.wav");
	}
}

void settings::update_switch_func(gui_switch_func& b)
{
	if (m1_pressed && CheckCollisionPointCircle(cursor, b.center, b.size)) {
		b.func();
		play_sound_effect("normal-hitnormal.wav");
	}
}

void settings::draw_switch(gui_switch& b)
{
	DrawCircleV(b.center, b.size, BLACK);
	DrawCircleLinesV(b.center, b.size, WHITE);

	if (b.option)
		DrawCircleV(b.center, b.size * 0.8f, WHITE);
	DrawTextEx(aller_r, b.desc.c_str(), { b.pos.x + b.size * 1.8f, b.pos.y }, b.size * 1.5f, 0, WHITE);
}

void settings::callback_update_volume_master(float value)
{
	SetMasterVolume(value / 100.0f);
}
void settings::callback_update_volume_music(float value)
{
	SetMusicVolume(music, value / 100.0f);
}
void settings::callback_update_volume_sound(float value)
{
	for(auto& ch : audio_channels)
	SetSoundVolume(ch, value / 100.0f);
}

void settings::update_slider(gui_slider& s)
{
	if (m1_pressed && CheckCollisionPointRec(cursor, s.pos_dim)) {
		selected_element = &s;
		s.held = true;
	}
	if (selected_element == &s) {
		if (m1_down) {
			s.value = s.min_value + (s.max_value - s.min_value) * std::max(0.0f, std::min((cursor.x - s.pos_dim.x) / s.pos_dim.width, 1.0f));

			s.value = ((float)((int)(s.value / s.fine_control_val + .5))) * s.fine_control_val;
			s.percentage = std::max(0.0f, std::min(((s.value - s.min_value) / (s.max_value - s.min_value)), 1.0f));
		}
		if (IsKeyPressed(KEY_RIGHT)) {
			s.value += s.fine_control_val;
			s.value = std::clamp(s.value, s.min_value, s.max_value);
			s.percentage = std::max(0.0f, std::min(((s.value - s.min_value) / (s.max_value - s.min_value)), 1.0f));
		}
		if (IsKeyPressed(KEY_LEFT)) {
			s.value -= s.fine_control_val;
			s.value = std::clamp(s.value, s.min_value, s.max_value);
			s.percentage = std::max(0.0f, std::min(((s.value - s.min_value) / (s.max_value - s.min_value)), 1.0f));
		}
		
	}

	if (m1_released) s.held = false;
}

void settings::update_slider_func(gui_slider_func& s)
{
	if (m1_pressed && CheckCollisionPointRec(cursor, s.pos_dim)) {
		selected_element = &s;
		s.held = true;
	}
	if (selected_element == &s) {
		if (m1_down) {
			s.value = s.min_value + (s.max_value - s.min_value) * std::max(0.0f, std::min((cursor.x - s.pos_dim.x) / s.pos_dim.width, 1.0f));

			s.value = ((float)((int)(s.value / s.fine_control_val + .5))) * s.fine_control_val;
			s.percentage = std::max(0.0f, std::min(((s.value - s.min_value) / (s.max_value - s.min_value)), 1.0f));

			s.func(s.value);
		}
		if (IsKeyPressed(KEY_RIGHT)) {
			s.value += s.fine_control_val;
			s.value = std::clamp(s.value, s.min_value, s.max_value);
			s.percentage = std::max(0.0f, std::min(((s.value - s.min_value) / (s.max_value - s.min_value)), 1.0f));

			s.func(s.value);
		}
		if (IsKeyPressed(KEY_LEFT)) {
			s.value -= s.fine_control_val;
			s.value = std::clamp(s.value, s.min_value, s.max_value);
			s.percentage = std::max(0.0f, std::min(((s.value - s.min_value) / (s.max_value - s.min_value)), 1.0f));

			s.func(s.value);
		}

	}

	if (m1_released) s.held = false;
	
}

void settings::draw_slider(gui_slider& s)
{
	Color c = GRAY;
	if (selected_element == &s) c = WHITE;
	DrawRectangleRec(s.pos_dim, BLACK);
	DrawRectangleLinesF(s.pos_dim.x, s.pos_dim.y, s.pos_dim.width, s.pos_dim.height, c);
	
	DrawRectangleV({ s.pos_dim.x, s.pos_dim.y }, { s.percentage * s.pos_dim.width, s.pos_dim.height }, c);
	DrawTextEx(aller_r, s.desc.c_str(), { s.pos_dim.x + s.pos_dim.width + 16 * screen_height_ratio, s.pos_dim.y }, 24 * screen_height_ratio, 0, c);

	std::string text = format_floats(s.value) + s.unit;
	auto text_length = MeasureTextEx(aller_r, text.c_str(), 24 * screen_height_ratio, 0);
	DrawTextEx(aller_r, text.c_str(), { s.pos_dim.x + s.pos_dim.width / 2.0f - text_length.x / 2.0f, s.pos_dim.y + s.pos_dim.height }, 24 * screen_height_ratio, 0, BLACK);
}

void settings::callback_update_resolution(int index) {
	update_screen_resolution(available_resolutions[index].x, available_resolutions[index].y);
}

void settings::update_dropdown_list(gui_dropdown_list& b) {
	if (m1_pressed) {
		if (CheckCollisionPointRec(cursor, { b.pos_dim.x, b.pos_dim.y, b.pos_dim.width, b.element_size })) {
			selected_element = &b;
			b.list_open = !b.list_open;
		}
		if (b.list_open) {
			if (!CheckCollisionPointRec(cursor, { b.pos_dim.x, b.pos_dim.y, b.pos_dim.width, b.pos_dim.height + b.element_size })) {
				b.list_open = false;
				b.scroll_pos = 0;
			}
		}
	}
	if (b.list_open) {

		float wheel = GetMouseWheelMove();
		if (wheel != 0.0f) {
			b.scroll_pos += wheel * b.element_size;
		}

		if (b.pos_dim.height > b.pos_dim.y) {
			float max_scroll = 0;
			float min_scroll = b.pos_dim.y - b.pos_dim.height;
			b.scroll_pos = std::clamp(b.scroll_pos, min_scroll, max_scroll);
		}
		else {
			b.scroll_pos = 0;
		}

		if (m1_pressed && CheckCollisionPointRec(cursor, { b.pos_dim.x, b.pos_dim.y, b.pos_dim.width, b.pos_dim.height + b.element_size })) {
			int begin_height = b.scroll_pos + b.pos_dim.y + b.element_size;
			if (cursor.y < begin_height) return;
			int index = (cursor.y - begin_height) / b.element_size;
			if (index >= b.options.size()) return;
			b.func(index);
		}
	}
}

void settings::draw_dropdown_list(gui_dropdown_list& b) {
	DrawRectangleRec({ b.pos_dim.x, b.pos_dim.y, b.pos_dim.width, b.element_size }, BLACK);

	Vector2 text_size = MeasureTextEx(aller_r, b.desc.c_str(), 24 * screen_height_ratio, 0);

	float switch_y = b.pos_dim.y + b.scroll_pos;
	float text_x = b.pos_dim.x + (b.pos_dim.width - text_size.x) * 0.5f;
	float text_y = b.pos_dim.y + (b.element_size - text_size.y) * 0.5f;


	DrawTextEx(aller_r, b.desc.c_str(), { text_x - 16 * screen_height_ratio, text_y }, 24 * screen_height_ratio, 0, WHITE);
	
	if (b.list_open) {
		DrawTriangle(
			{ b.pos_dim.x + b.pos_dim.width * 0.82f, b.pos_dim.y + 0.7f * b.element_size },
			{ b.pos_dim.x + b.pos_dim.width * 0.94f, b.pos_dim.y + 0.7f * b.element_size },
			{ b.pos_dim.x + b.pos_dim.width * 0.88f, b.pos_dim.y + 0.2f * b.element_size }, WHITE);
		DrawRectangleRec({ b.pos_dim.x, b.pos_dim.y + b.element_size, b.pos_dim.width, b.pos_dim.height + b.scroll_pos }, BLACK);

		for (int i = 0; i < b.options.size(); i++) {
			Vector2 text_size = MeasureTextEx(aller_r, b.options[i].c_str(), 24 * screen_height_ratio, 0);
			float text_x = b.pos_dim.x + (b.pos_dim.width - text_size.x) * 0.5f;
			float text_y = (i + 1) * b.element_size + switch_y + (b.element_size - text_size.y) * 0.5f;
			if (text_y < b.pos_dim.y + b.element_size - 4) continue;
			DrawTextEx(aller_r, b.options[i].c_str(), { text_x, text_y }, 24 * screen_height_ratio, 0, WHITE);
		}
	}
	else {
		DrawTriangle(
			{ b.pos_dim.x + b.pos_dim.width * 0.88f, b.pos_dim.y + 0.7f * b.element_size },
			{ b.pos_dim.x + b.pos_dim.width * 0.94f, b.pos_dim.y + 0.2f * b.element_size },
			{ b.pos_dim.x + b.pos_dim.width * 0.82f, b.pos_dim.y + 0.2f * b.element_size }, WHITE);
	}

	
}

