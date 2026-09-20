#pragma once

struct ImDrawList;
class Game;

// Floating "Spectators" panel (background draw list, non-interactive).
// Lists controller names whose observer target is the local pawn.
// Empty list draws a dimmed "No spectators" hint instead of hiding.
void draw_spectators(ImDrawList* dl, const Game& game, float screen_w, float screen_h);
