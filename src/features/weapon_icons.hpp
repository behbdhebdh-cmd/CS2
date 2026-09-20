#pragma once

#include <map>
#include <string>
#include <cstdint>

#ifndef $
#define $(str) str
#endif

namespace weapon_icons {

// Required weapon_icon_table as std::map<std::wstring, std::wstring>
extern const std::map<std::wstring, std::wstring> weapon_icon_table;

// Resolves weapon by class name (e.g. "ak47", "weapon_ak47", "awp", "deagle")
// and/or item definition index (e.g. 7, 9, 1), returning an ImGui-compatible UTF-8 glyph.
std::string resolve(const std::string& weapon_class, uint16_t def_index = 0);

// Resolves wide class name to wide glyph
std::wstring resolve_wide(const std::wstring& weapon_class);

// Converts wide string to UTF-8
std::string wide_to_utf8(const std::wstring& w);

} // namespace weapon_icons
