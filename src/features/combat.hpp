#pragma once

struct ImDrawList;
class Game;
class VisCheck;
struct CombatProfile;

enum class WpnClass {
    Rifle = 0,
    Pistol = 1,
    Sniper = 2,
    Utility = 3,
};

WpnClass classify_weapon(int def);
const char* weapon_class_name(WpnClass c);
const CombatProfile& combat_active_profile(const Game& game);

void combat_tick(const Game& game, const VisCheck& vis, float dt, bool menu_open);
void combat_draw(ImDrawList* dl, const Game& game, float screen_w, float screen_h);
