#pragma once

struct ImDrawList;
class Game;

// Damage tracker (health-diff polling) + center hitmarker + killfeed corner.
// Zones (head/chest/body) are a heuristic: when the damaged pawn matches the
// current aim lock, the locked bone decides; otherwise the hit counts as body.
// Kill rows appear when a recently damaged pawn vanishes from the snapshot.
void hitlog_tick(const Game& game);
void hitlog_draw(ImDrawList* dl, const Game& game, float screen_w, float screen_h);
