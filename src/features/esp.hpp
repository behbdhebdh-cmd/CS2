#pragma once

#include "sdk/game.hpp"

struct ImDrawList;
class VisCheck;

void draw_players(ImDrawList* dl, const Game& game, const VisCheck& vis, float screen_w, float screen_h, float time_s);
void draw_watermark(ImDrawList* dl, const Game& game, const VisCheck& vis, float screen_w);
