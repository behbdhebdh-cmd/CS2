#pragma once

#include "imgui.h"

// Shared anchor math for floating overlay panels (spectators, damage log).
// Anchor: 0 = top-right, 1 = top-left, 2 = bottom-left, 3 = bottom-right.
// ox/oy nudge the panel in pixels after anchoring. 16 px screen margin.
inline ImVec2 panel_anchor(int anchor, float w, float h, float sw, float sh, float ox, float oy)
{
    const float m = 16.f;
    float x = sw - m - w;
    float y = m;
    switch (anchor) {
    case 1: x = m; break;
    case 2: x = m; y = sh - m - h; break;
    case 3: y = sh - m - h; break;
    default: break; // 0: top-right
    }
    return ImVec2(x + ox, y + oy);
}
