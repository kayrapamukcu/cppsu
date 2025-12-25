#pragma once

#include "raylib.h"
#include <cmath>
#include <vector>
#include <string>
#include <format>
#include <fstream>
#include <chrono>
#include <unordered_map>
#include <array>
#include <iostream>

// Enums and structs

enum SAMPLE_SETS {
	SAMPLE_SET_NORMAL,
	SAMPLE_SET_SOFT,
	SAMPLE_SET_DRUM
};

enum RANKS{
	RANK_F,
	RANK_D,
	RANK_C,
	RANK_B,
	RANK_A,
	RANK_S,
	RANK_X,
	RANK_SH,
	RANK_XH
};

enum GAME_STATES {
	INIT,
	MAIN_MENU,
	SETTINGS,
	IMPORTING,
	SONG_SELECT,
	INGAME,
	RESULT_SCREEN
};

struct file_struct {
	std::string audio_filename;
	std::string title;
	std::string artist;
	std::string creator;
	std::string difficulty;
	std::string bg_photo_name;
	std::string osu_filename;

	int preview_time = 0;
	int beatmap_set_id = -1;
	int beatmap_id = -1;

	float hp = 5.0f;
	float cs = 4.0f;
	float od = 8.0f;
	float ar = 9.0f;
	float slider_multiplier = 1.4f;
	float slider_tickrate = 1.0f;

	float star_rating = 5.0f;

	float avg_bpm = 120.0f;
	float min_bpm = 120.0f;
	float max_bpm = 120.0f;

	uint16_t map_length = 0;
	uint16_t circle_count = 0;
	uint16_t slider_count = 0;
	uint16_t spinner_count = 0;

	float stack_leniency = 0.5f;
	SAMPLE_SETS sample_set = SAMPLE_SET_NORMAL;

	std::string source;
	int mode = 0;
};

struct results_struct {
	std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds> time;
	std::string player_name;
	std::string beatmap_header;
	std::string beatmap_header_2;
	uint32_t score = 0;
	uint32_t max_combo = 0;
	uint32_t hit300s = 0;
	uint32_t hit100s = 0;
	uint32_t hit50s = 0;
	uint32_t misses = 0;

	uint32_t geki = 0;
	uint32_t katu = 0;

	float accuracy = 0.0f;
	RANKS rank = RANK_F;
};

struct TexWithSrc {
	Texture2D tex{};
	Rectangle src{ 0,0,0,0 };
};

struct Notice {
	std::string text;
	float time_left;
};

// Settings variables

inline float screen_width = 1024.0f;
inline float screen_height = 768.0f;

inline bool settings_sliderend_rendering = false;
inline float settings_mouse_scale = 1.0f;

inline std::string player_name = "Player";

// Audio stuff

constexpr int MAX_AUDIO_CHANNELS = 24;

inline std::unordered_map<std::string, Sound> sound_effects;

inline int audio_on_channel = 0;
inline std::array<Sound, MAX_AUDIO_CHANNELS> audio_channels; // Sound, is_playing

static inline void play_sound_effect(const std::string& name, float volume = 1.0f) {
	if (sound_effects.find(name) == sound_effects.end()) {
		std::cout << "Tried to play sound effect that doesn't exist: " << name << "\n";
		return;
	}
	UnloadSoundAlias(audio_channels[audio_on_channel]);
	audio_channels[audio_on_channel] = LoadSoundAlias(sound_effects[name]);
	SetSoundVolume(audio_channels[audio_on_channel], volume);
	PlaySound(audio_channels[audio_on_channel]);
	audio_on_channel = (audio_on_channel + 1) % MAX_AUDIO_CHANNELS;
}

// Global variables



inline constexpr int DB_VERSION = 8;
inline constexpr std::string_view CLIENT_VERSION = "a2025.1219";

inline float screen_width_ratio = (float)screen_width / 1024.0f;
inline float screen_height_ratio = (float)screen_height / 768.0f;

inline float screen_scale = std::min(screen_width_ratio, screen_height_ratio);

inline bool isNPOTSupported = false;
inline GAME_STATES game_state = INIT;
inline bool importing_map = false;

inline Music music;
inline TexWithSrc background;
inline Texture song_select_top_bar;
inline Texture atlas;

inline std::vector<Rectangle> tex;

inline float playfield_scale = screen_height / 480.0f;
inline float playfield_offset_x = (screen_width - 512.0f * playfield_scale) / 2.0f;
inline float playfield_offset_y = (screen_height - 384.0f * playfield_scale) / 2.0f + 8.0f * playfield_scale;

inline Font aller_r;
inline Font aller_l;
inline Font aller_b;

class ingame;
inline ingame* g_ingame = nullptr;
class result_screen;
inline result_screen* g_result_screen = nullptr;

inline std::vector<Notice> notices;

inline bool key1_down = false;
inline bool key2_down = false;

inline KeyboardKey key_1 = KEY_C;
inline KeyboardKey key_2 = KEY_V;

// Helper functions

static inline void split_sv_fixed(std::string_view sv, std::string_view* out, int max_parts) {
	int idx = 0;
	size_t start = 0;
	while (idx < max_parts) {
		size_t pos = sv.find(',', start);
		if (pos == std::string_view::npos) {
			out[idx++] = sv.substr(start);
			break;
		}
		out[idx++] = sv.substr(start, pos - start);
		start = pos + 1;
	}
}

static inline std::vector<std::string> split_string(const std::string& s, char delimiter) {
	std::vector<std::string> tokens;
	std::string token;
	for (char c : s) {
		if (c == delimiter) {
			tokens.push_back(token);
			token.clear();
		} else {
			token += c;
		}
	}
	tokens.push_back(token);
	return tokens;
}

static inline std::string get_score_string(uint32_t score) {
	std::string score_str = std::to_string(score);
	auto n = score_str.length();
	if (n < 8) score_str.insert(0, 8 - n, '0');
	return score_str;
}

static inline bool roughly_equal(float a, float b) {
	return std::fabs(a - b) <= 1.5f * playfield_scale;
}

static inline bool roughly_equal(Vector2 a, Vector2 b) {
	return roughly_equal(a.x, b.x) && roughly_equal(a.y, b.y);
}

static inline void chomp_cr(std::string& s) {
	// CHOMP
	if (!s.empty() && s.back() == '\r') s.pop_back();
}

static inline void to_int(std::string_view s, int& out) {
	std::from_chars(s.data(), s.data() + s.size(), out, 10);
}

static inline void to_float(std::string_view s, float& out) {
	std::from_chars(s.data(), s.data() + s.size(), out);
}

static inline std::string format_floats(float v) {
	std::string s = std::format("{:.2f}", v);
	while (!s.empty() && s.back() == '0') s.pop_back();
	if (!s.empty() && s.back() == '.') s.pop_back();
	return s;
}

static inline std::string format_length(uint16_t length) {
	int minutes = length / 60;
	int seconds = length % 60;
	return std::string(minutes < 10 ? "0" : "") + std::to_string(minutes) + ":" + (seconds < 10 ? "0" : "") + std::to_string(seconds);
}

static inline bool IsPOT(int v) { return v > 0 && (v & (v - 1)) == 0; }
static inline int NextPOT(int v) { if (v <= 1) return 1; v--; v |= v >> 1; v |= v >> 2; v |= v >> 4; v |= v >> 8; v |= v >> 16; return v + 1; }

static inline void DrawBackgroundCompat() {
	if (!isNPOTSupported) {
		DrawTexture(background.tex, 0, 0, WHITE);
	} else
		DrawTexturePro(background.tex, background.src, { 0,0, (float)screen_width, (float)screen_height }, { 0,0 }, 0.0f, WHITE);
}

static inline void DrawTextureCompatPro(const TexWithSrc& t, Rectangle dst, Color tint) {
	DrawTexturePro(t.tex, t.src, dst, { 0,0 }, 0.0f, tint);
}

static inline void DrawTextureCompat(const TexWithSrc& t, Vector2 pos, Color tint) {
	DrawTexturePro(t.tex, t.src, { pos.x, pos.y, t.src.width, t.src.height }, { 0,0 }, 0.0f, tint);
}

static inline float ShiftAngleForward(float angle) {
	while (angle < 0.0f) angle += 360.0f;
	while (angle >= 360.0f) angle -= 360.0f;
	return angle;
}

static inline float ShiftRadiansForward(float angle, float ref) {
	while (angle < ref) angle += 2.0f * PI;
	while (angle >= ref + 2 * PI) angle -= 2.0f * PI;
	return angle;
}

inline std::vector<unsigned char> g_musicData;

static Music LoadMusicStreamFromRam(const char* path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        TraceLog(LOG_ERROR, "Failed to open music file: %s", path);
        return {0};
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    g_musicData.resize((size_t)size);
    if (!file.read(reinterpret_cast<char*>(g_musicData.data()), size)) {
        TraceLog(LOG_ERROR, "Failed to read music file: %s", path);
        return {0};
    }

    // choose correct extension
    const char* ext = nullptr;
    std::string pathstr = path;
    if (pathstr.ends_with(".ogg")) ext = ".ogg";
    else if (pathstr.ends_with(".mp3")) ext = ".mp3";
    else ext = ".wav";

    // load from RAM buffer
    Music m = LoadMusicStreamFromMemory(ext, g_musicData.data(), (int)g_musicData.size());
    return m;
}

static void UnloadMusicStreamFromRam(Music& music) {
    UnloadMusicStream(music);
    g_musicData.clear();
}

static TexWithSrc LoadTextureCompat(const std::string& path) {
	TexWithSrc out;
	Image img = LoadImage(path.c_str());
	const int origW = img.width, origH = img.height;

	const bool isNPOT = !IsPOT(img.width) || !IsPOT(img.height);

	if (isNPOT && !isNPOTSupported) {
		ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
		ImageResizeCanvas(&img, NextPOT(img.width), NextPOT(img.height), 0, 0, BLANK);
	}

	out.tex = LoadTextureFromImage(img);
	UnloadImage(img);
	out.src = { 0, 0, (float)origW, (float)origH };

	return out;
}

static TexWithSrc LoadBackgroundCompat(const std::string& path) {
	TexWithSrc out{};
	Image img = LoadImage(path.c_str());
	if (!img.data) return out;

	const bool isNPOT = !IsPOT(img.width) || !IsPOT(img.height);
	if (isNPOT && !isNPOTSupported) {
		ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8);
		ImageResizeNN(&img, 1024, 768);
	}

	out.tex = LoadTextureFromImage(img);
	UnloadImage(img);

	out.src = { 0, 0, (float)out.tex.width, (float)out.tex.height };

	SetTextureWrap(out.tex, TEXTURE_WRAP_CLAMP);
	SetTextureFilter(out.tex, TEXTURE_FILTER_POINT);

	return out;
}

static void DrawTextExScaled(Font font, const char* text, Vector2 position, float fontSize, float spacing, Color tint) {
	DrawTextEx(font, text, { position.x * screen_width_ratio, position.y * screen_height_ratio }, fontSize * screen_scale, spacing, tint);
}