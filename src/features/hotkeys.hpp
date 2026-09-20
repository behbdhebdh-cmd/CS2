#pragma once

struct ImDrawList;

// Floating "Hotkeys" panel (background draw list, non-interactive).
// Reads g_menu live every frame: ESP / Aimbot / Triggerbot enable state
// plus the configured aim/trigger keys. No own state, always in sync.
void draw_hotkeys(ImDrawList* dl);

// "ALT", "Shift", "Mouse5", "F3", "G", ... into out (always terminated).
void hotkey_name(int vk, char* out, int out_size);
