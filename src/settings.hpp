#pragma once

#include <vector>
#include "raylib.h"
#include "globals.hpp"

class settings {
public:
	static void update_screen_resolution(int width, int height);
	static void init();
	static void update();
private:
	static inline std::vector<Vector2> available_resolutions;
	static inline float settings_resolution_scroll_offset = 0.0f;
	static inline Vector2 selected_resolution = { screen_width, screen_height };
};