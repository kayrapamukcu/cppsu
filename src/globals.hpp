#pragma once

#include "raylib.h"
#include <vector>
#include <string>
#include <format>
#include <fstream>

// Enums and structs

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
};

enum GAME_STATES {
	MAIN_MENU,
	SETTINGS,
	IMPORTING,
	SONG_SELECT,
	INGAME,
	SCORE_SCREEN
};

struct TexWithSrc {
	Texture2D tex{};
	Rectangle src{ 0,0,0,0 };
};

struct Notice {
	std::string text;
	float time_left;
};

// Global variables

inline constexpr int DB_VERSION = 7;
inline constexpr std::string_view CLIENT_VERSION = "a2025.1101";

inline float screen_width = 1024;
inline float screen_height = 768;
inline bool isNPOTSupported = false;
inline GAME_STATES game_state = MAIN_MENU;
inline bool importing_map = false;

inline Music music;
inline TexWithSrc background;
inline TexWithSrc song_select_top_bar;

inline float playfield_scale = screen_height / 480.0f;
inline float playfield_offset_x = (screen_width - 512.0f * playfield_scale) / 2.0f;
inline float playfield_offset_y = (screen_height - 384.0f * playfield_scale) / 2.0f + 8.0f * playfield_scale;

inline Font font12;
inline Font font24;
inline Font font36;

inline Font font12_b;
inline Font font24_b;
inline Font font36_b;

inline Font font12_l;
inline Font font24_l;
inline Font font36_l;

inline KeyboardKey k_1 = KEY_C;
inline KeyboardKey k_2 = KEY_V;

inline bool k1_down = false;
inline bool k2_down = false;

class ingame;
inline ingame* g_ingame = nullptr;

inline std::vector<Notice> notices;

// Helper functions

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
		DrawTexturePro(background.tex, background.src, { 0,0, screen_width, screen_height }, { 0,0 }, 0.0f, WHITE);
}

static inline void DrawTextureCompatPro(const TexWithSrc& t, Rectangle dst, Color tint) {
	DrawTexturePro(t.tex, t.src, dst, { 0,0 }, 0.0f, tint);
}

static inline void DrawTextureCompat(const TexWithSrc& t, Vector2 pos, Color tint) {
	DrawTexturePro(t.tex, t.src, { pos.x, pos.y, t.src.width, t.src.height }, { 0,0 }, 0.0f, tint);
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

    g_musicData.resize(size);
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