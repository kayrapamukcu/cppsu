#pragma once

#include <vector>
#include "raylib.h"
#include "globals.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

class settings {
public:
	static void update_screen_resolution(int width, int height);
	static void init();
	static void update();
	static void go_back();
private:
	struct gui_button {
		Vector2 pos;
		Vector2 center;
		float size;
		bool& option;

		std::string desc;
	};

	struct gui_button_func : gui_button{
		void(*func)(void);
	};

	struct gui_slider {
		Rectangle pos_dim;
		float percentage;
		float min_value;
		float max_value;
		float fine_control_val;
		float& value;

		bool held;

		std::string desc;
		std::string unit;
	};

	struct gui_textfield {
		Rectangle pos_dim;

		std::string desc;
		std::string& value;
	};

	struct gui_dropdown_list {
		Rectangle pos_dim;
		float scroll_pos;
		uint32_t option_chosen_idx;
		float element_size;
		bool list_open;

		void(*func)(int);
		std::string desc;
		std::vector<std::string> options;
	};

	struct gui_changekey {
		Vector2 pos;
		Vector2 dim;

		KeyboardKey& key;
	};

	static inline std::vector<Vector2> available_resolutions;
	static inline float settings_resolution_scroll_offset = 0.0f;
	static inline Vector2 selected_resolution = { screen_width, screen_height };
	static inline bool initialized = false;

	static inline std::vector<gui_button> buttons;
	static inline std::vector<gui_button_func> buttons_func;
	static inline std::vector<gui_slider> sliders;
	static inline std::vector<gui_textfield> textfields;
	static inline std::vector<gui_dropdown_list> lists;

	static inline void* selected_element;

	static void callback_raw_input();
	static void callback_update_resolution(int index);

	static void update_button(gui_button& b);
	static void update_button_func(gui_button_func& b);
	static void draw_button(gui_button& b);

	static void update_slider(gui_slider& s);
	static void draw_slider(gui_slider& s);

	static void update_dropdown_list(gui_dropdown_list& b);
	static void draw_dropdown_list(gui_dropdown_list& b);
	
};