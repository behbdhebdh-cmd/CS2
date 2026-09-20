#pragma once

struct ImDrawList;
class Game;

// Externes 2D-Radar (heading-up oder north-up), eigene Persistenz in
// radar.json neben der EXE. Das Config-System wird bewusst nicht benutzt.
struct RadarSettings {
    bool enable = true;
    int anchor = 0;        // 0 TR, 1 TL, 2 BL, 3 BR (panel_anchor)
    int off_x = 0;
    int off_y = 130;       // unter der Spectator-Liste (TR)
    int size = 200;        // Panel-Breite px, 140..320
    float opacity = 0.95f; // 0.30..1.00
    float range = 30.f;    // Meter am Scope-Rand, 10..60
    bool rotate = true;    // true = heading-up, false = north-up
    int filter = 0;        // 0 alle, 1 nur Gegner, 2 nur Team
    float enemy[4] = { 0.94f, 0.43f, 0.46f, 1.f };
    float team[4]  = { 0.55f, 0.78f, 0.92f, 1.f };
    float local[4] = { 0.96f, 0.97f, 0.99f, 1.f };
    float ring[4]  = { 1.00f, 1.00f, 1.00f, 1.f };
};

extern RadarSettings g_radar;

void radar_startup();   // radar.json laden (stumm bei Fehlen)
void radar_mark_dirty();// Einstellungen geaendert -> verzögert speichern
void radar_housekeep(bool menu_open); // Autosave wenn Menue zu + idle
void radar_save_now();  // sofort speichern
void radar_draw(ImDrawList* dl, const Game& game, float screen_w, float screen_h);
