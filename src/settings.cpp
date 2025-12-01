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
}

void settings::init() {
	// Get resolutions
	GLFWmonitor* mon = glfwGetPrimaryMonitor();

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
}

void settings::update() {

	// add cursor size control

	auto mouse_pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
	Vector2 mouse_location = GetMousePosition();

	DrawRectangleGradientV(0, 0, (int)screen_width, (int)screen_height, BLUE, DARKBLUE);
	if (IsKeyPressed(KEY_B)) {
		game_state = MAIN_MENU;
	}

	// display resolutions
	auto res_text_size = MeasureTextEx(aller_r, "Resolutions", 48 * screen_scale, 0);
	auto res_subtext_size = MeasureTextEx(aller_r, "(You can scroll)", 24 * screen_scale, 0);

	auto res_text_x = 783 * screen_width_ratio + (208 * screen_width_ratio - res_text_size.x) * 0.5f;
	auto res_subtext_x = 783 * screen_width_ratio + (208 * screen_width_ratio - res_subtext_size.x) * 0.5f;

	DrawRectangle(750 * screen_width_ratio, 0, screen_width - 684 * screen_width_ratio, screen_height, GRAY);
	DrawRectangle(750 * screen_width_ratio, 0, 10 * screen_width_ratio, screen_height, BLACK);
	DrawTextExScaled(aller_r, "Resolutions", { 783, 8 + settings_resolution_scroll_offset }, 48, 0, WHITE);
	DrawTextExScaled(aller_r, "(You can scroll)", { 830, 40 + settings_resolution_scroll_offset }, 24, 0, WHITE);

	float wheel = GetMouseWheelMove();
	if (wheel != 0.0f) {
		settings_resolution_scroll_offset += wheel * 30.0f;
	}

	float item_height = 36 * screen_height_ratio;
	float list_height = available_resolutions.size() * item_height;
	float viewport_height = screen_height - 64 * screen_height_ratio;

	if (list_height > viewport_height) {
		float max_scroll = 0;
		float min_scroll = viewport_height - list_height;
		settings_resolution_scroll_offset = std::clamp(settings_resolution_scroll_offset, min_scroll, max_scroll);
	}
	else {
		settings_resolution_scroll_offset = 0;
	}

	for (size_t i = 0; i < available_resolutions.size(); i++) {
		auto& res = available_resolutions[i];
		std::string res_str = std::to_string((int)res.x) + "x" + std::to_string((int)res.y);
		Color col = (res.x == selected_resolution.x && res.y == selected_resolution.y) ? YELLOW : WHITE;

		float button_width = 208 * screen_width_ratio;
		float button_x = 783 * screen_width_ratio;
		float button_y = (64 + i * 36) * screen_height_ratio + settings_resolution_scroll_offset;
		float font_size = 24 * screen_scale;
		Vector2 text_size = MeasureTextEx(aller_r, res_str.c_str(), font_size, 0);
		float text_x = button_x + (button_width - text_size.x) * 0.5f;
		float text_y = button_y + (32 * screen_scale - text_size.y) * 0.5f;

		DrawRectangle(button_x, button_y, button_width, 32 * screen_scale, BLACK);
		DrawTextEx(aller_r, res_str.c_str(), { text_x, text_y }, font_size, 0, col);

		if (CheckCollisionPointRec(mouse_location, { button_x, button_y, button_width, 32 * screen_scale })) {
			if(mouse_pressed) 
				update_screen_resolution(res.x, res.y);
		}
	}
}