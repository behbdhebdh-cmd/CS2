#include "features/weapon_icons.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include <algorithm>
#include <cwctype>

namespace weapon_icons {

// Provided weapon_icon_table using $() prefix encryption marker
const std::map<std::wstring, std::wstring> weapon_icon_table = {
    // Pistols
    { $(L"weapon_deagle"),        $(L"\uE001") },
    { $(L"deagle"),               $(L"\uE001") },
    { $(L"weapon_elite"),         $(L"\uE002") },
    { $(L"elite"),                $(L"\uE002") },
    { $(L"weapon_fiveseven"),     $(L"\uE003") },
    { $(L"fiveseven"),            $(L"\uE003") },
    { $(L"weapon_glock"),         $(L"\uE004") },
    { $(L"glock"),                $(L"\uE004") },
    { $(L"weapon_hkp2000"),       $(L"\uE013") },
    { $(L"hkp2000"),              $(L"\uE013") },
    { $(L"p2000"),                $(L"\uE013") },
    { $(L"weapon_p250"),          $(L"\uE020") },
    { $(L"p250"),                 $(L"\uE020") },
    { $(L"weapon_tec9"),          $(L"\uE01E") },
    { $(L"tec9"),                 $(L"\uE01E") },
    { $(L"weapon_cz75a"),         $(L"\uE03F") },
    { $(L"cz75a"),                $(L"\uE03F") },
    { $(L"weapon_usp_silencer"),  $(L"\uE03D") },
    { $(L"usp_silencer"),         $(L"\uE03D") },
    { $(L"usp"),                  $(L"\uE03D") },
    { $(L"weapon_revolver"),      $(L"\uE040") },
    { $(L"revolver"),             $(L"\uE040") },
    { $(L"weapon_taser"),         $(L"\uE01F") },
    { $(L"taser"),                $(L"\uE01F") },
    { $(L"zeus"),                 $(L"\uE01F") },

    // Rifles & Snipers
    { $(L"weapon_ak47"),          $(L"\uE007") },
    { $(L"ak47"),                 $(L"\uE007") },
    { $(L"weapon_aug"),           $(L"\uE008") },
    { $(L"aug"),                  $(L"\uE008") },
    { $(L"weapon_awp"),           $(L"\uE009") },
    { $(L"awp"),                  $(L"\uE009") },
    { $(L"weapon_famas"),         $(L"\uE00A") },
    { $(L"famas"),                $(L"\uE00A") },
    { $(L"weapon_g3sg1"),         $(L"\uE00B") },
    { $(L"g3sg1"),                $(L"\uE00B") },
    { $(L"weapon_galilar"),       $(L"\uE00D") },
    { $(L"galilar"),              $(L"\uE00D") },
    { $(L"galil"),                $(L"\uE00D") },
    { $(L"weapon_m4a1"),          $(L"\uE00E") },
    { $(L"m4a1"),                 $(L"\uE00E") },
    { $(L"m4a4"),                 $(L"\uE00E") },
    { $(L"weapon_m4a1_silencer"), $(L"\uE010") },
    { $(L"m4a1_silencer"),        $(L"\uE010") },
    { $(L"m4a1-s"),               $(L"\uE010") },
    { $(L"weapon_scar20"),        $(L"\uE026") },
    { $(L"scar20"),               $(L"\uE026") },
    { $(L"weapon_sg556"),         $(L"\uE027") },
    { $(L"sg556"),                $(L"\uE027") },
    { $(L"sg553"),                $(L"\uE027") },
    { $(L"weapon_ssg08"),         $(L"\uE028") },
    { $(L"ssg08"),                $(L"\uE028") },
    { $(L"scout"),                $(L"\uE028") },

    // SMGs
    { $(L"weapon_mac10"),         $(L"\uE011") },
    { $(L"mac10"),                $(L"\uE011") },
    { $(L"weapon_p90"),           $(L"\uE024") },
    { $(L"p90"),                  $(L"\uE024") },
    { $(L"weapon_mp5sd"),         $(L"\uE011") },
    { $(L"mp5sd"),                $(L"\uE011") },
    { $(L"weapon_ump45"),         $(L"\uE018") },
    { $(L"ump45"),                $(L"\uE018") },
    { $(L"ump"),                  $(L"\uE018") },
    { $(L"weapon_bizon"),         $(L"\uE01A") },
    { $(L"bizon"),                $(L"\uE01A") },
    { $(L"weapon_mp7"),           $(L"\uE021") },
    { $(L"mp7"),                  $(L"\uE021") },
    { $(L"weapon_mp9"),           $(L"\uE022") },
    { $(L"mp9"),                  $(L"\uE022") },

    // Heavy & Shotguns
    { $(L"weapon_xm1014"),        $(L"\uE019") },
    { $(L"xm1014"),               $(L"\uE019") },
    { $(L"weapon_mag7"),          $(L"\uE01B") },
    { $(L"mag7"),                 $(L"\uE01B") },
    { $(L"weapon_negev"),         $(L"\uE01C") },
    { $(L"negev"),                $(L"\uE01C") },
    { $(L"weapon_sawedoff"),      $(L"\uE01D") },
    { $(L"sawedoff"),             $(L"\uE01D") },
    { $(L"weapon_nova"),          $(L"\uE023") },
    { $(L"nova"),                 $(L"\uE023") },
    { $(L"weapon_m249"),          $(L"\uE03C") },
    { $(L"m249"),                 $(L"\uE03C") },

    // Grenades & Utility
    { $(L"weapon_flashbang"),     $(L"\uE02B") },
    { $(L"flashbang"),            $(L"\uE02B") },
    { $(L"weapon_hegrenade"),     $(L"\uE02C") },
    { $(L"hegrenade"),            $(L"\uE02C") },
    { $(L"weapon_smokegrenade"),  $(L"\uE02D") },
    { $(L"smokegrenade"),         $(L"\uE02D") },
    { $(L"smoke"),                $(L"\uE02D") },
    { $(L"weapon_molotov"),       $(L"\uE02E") },
    { $(L"molotov"),              $(L"\uE02E") },
    { $(L"weapon_decoy"),         $(L"\uE02F") },
    { $(L"decoy"),                $(L"\uE02F") },
    { $(L"weapon_incgrenade"),    $(L"\uE030") },
    { $(L"incgrenade"),           $(L"\uE030") },
    { $(L"weapon_c4"),            $(L"\uE031") },
    { $(L"c4"),                   $(L"\uE031") },

    // Knives
    { $(L"weapon_knife"),         $(L"\uE02A") },
    { $(L"knife"),                $(L"\uE02A") },
    { $(L"weapon_knife_t"),       $(L"\uE03B") },
    { $(L"knife_t"),              $(L"\uE03B") },
    { $(L"weapon_bayonet"),       $(L"\uE1F4") },
    { $(L"knife_bayonet"),        $(L"\uE1F4") },
    { $(L"bayonet"),              $(L"\uE1F4") },
    { $(L"weapon_knife_flip"),    $(L"\uE1F9") },
    { $(L"knife_flip"),           $(L"\uE1F9") },
    { $(L"weapon_knife_gut"),     $(L"\uE1FA") },
    { $(L"knife_gut"),            $(L"\uE1FA") },
    { $(L"weapon_knife_karambit"),$(L"\uE1FB") },
    { $(L"knife_karambit"),       $(L"\uE1FB") },
    { $(L"karambit"),             $(L"\uE1FB") },
    { $(L"weapon_knife_m9_bayonet"), $(L"\uE1FC") },
    { $(L"knife_m9_bayonet"),     $(L"\uE1FC") },
    { $(L"m9_bayonet"),           $(L"\uE1FC") },
    { $(L"weapon_knife_tactical"),$(L"\uE1FD") },
    { $(L"knife_tactical"),       $(L"\uE1FD") },
    { $(L"huntsman"),             $(L"\uE1FD") },
    { $(L"weapon_knife_falchion"),$(L"\uE200") },
    { $(L"knife_falchion"),       $(L"\uE200") },
    { $(L"weapon_knife_survival_bowie"), $(L"\uE202") },
    { $(L"knife_survival_bowie"), $(L"\uE202") },
    { $(L"bowie"),                $(L"\uE202") },
    { $(L"weapon_knife_butterfly"),$(L"\uE203") },
    { $(L"knife_butterfly"),      $(L"\uE203") },
    { $(L"butterfly"),            $(L"\uE203") },
    { $(L"weapon_knife_push"),    $(L"\uE204") },
    { $(L"knife_push"),           $(L"\uE204") },
    { $(L"shadow_daggers"),       $(L"\uE204") },
};

static const std::map<uint16_t, std::wstring> kDefIndexToName = {
    { 1, L"deagle" }, { 2, L"elite" }, { 3, L"fiveseven" }, { 4, L"glock" },
    { 7, L"ak47" }, { 8, L"aug" }, { 9, L"awp" }, { 10, L"famas" },
    { 11, L"g3sg1" }, { 13, L"galilar" }, { 14, L"m249" }, { 16, L"m4a1" },
    { 17, L"mac10" }, { 19, L"p90" }, { 23, L"mp5sd" }, { 24, L"ump45" },
    { 25, L"xm1014" }, { 26, L"bizon" }, { 27, L"mag7" }, { 28, L"negev" },
    { 29, L"sawedoff" }, { 30, L"tec9" }, { 31, L"taser" }, { 32, L"hkp2000" },
    { 33, L"mp7" }, { 34, L"mp9" }, { 35, L"nova" }, { 36, L"p250" },
    { 38, L"scar20" }, { 39, L"sg556" }, { 40, L"ssg08" },
    { 42, L"knife" }, { 43, L"flashbang" }, { 44, L"hegrenade" },
    { 45, L"smokegrenade" }, { 46, L"molotov" }, { 47, L"decoy" },
    { 48, L"incgrenade" }, { 49, L"c4" }, { 59, L"knife_t" },
    { 60, L"m4a1_silencer" }, { 61, L"usp_silencer" }, { 63, L"cz75a" }, { 64, L"revolver" },
    { 500, L"knife_bayonet" }, { 503, L"knife" }, { 505, L"knife_flip" },
    { 506, L"knife_gut" }, { 507, L"knife_karambit" }, { 508, L"knife_m9_bayonet" },
    { 509, L"knife_tactical" }, { 512, L"knife_falchion" }, { 514, L"knife_survival_bowie" },
    { 515, L"knife_butterfly" }, { 516, L"knife_push" },
    { 517, L"knife" }, { 518, L"knife" }, { 519, L"knife" }, { 520, L"knife" },
    { 521, L"knife" }, { 522, L"knife" }, { 523, L"knife" }, { 525, L"knife" }
};

std::string wide_to_utf8(const std::wstring& w)
{
    if (w.empty())
        return {};
    const int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
    if (len <= 0)
        return {};
    std::string s(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), static_cast<int>(w.size()), s.data(), len, nullptr, nullptr);
    return s;
}

static std::wstring to_lower_wide(std::wstring s)
{
    for (wchar_t& c : s)
        c = static_cast<wchar_t>(std::towlower(c));
    return s;
}

std::wstring resolve_wide(const std::wstring& weapon_class)
{
    if (weapon_class.empty())
        return {};

    const std::wstring lower = to_lower_wide(weapon_class);

    // 1. Direct match
    auto it = weapon_icon_table.find(lower);
    if (it != weapon_icon_table.end())
        return it->second;

    // 2. Without "weapon_" prefix
    if (lower.rfind(L"weapon_", 0) == 0) {
        const std::wstring stripped = lower.substr(7);
        auto it2 = weapon_icon_table.find(stripped);
        if (it2 != weapon_icon_table.end())
            return it2->second;
    } else {
        // With "weapon_" prefix added
        const std::wstring added = L"weapon_" + lower;
        auto it3 = weapon_icon_table.find(added);
        if (it3 != weapon_icon_table.end())
            return it3->second;
    }

    return {};
}

std::string resolve(const std::string& weapon_class, uint16_t def_index)
{
    // Try by class name
    if (!weapon_class.empty()) {
        const int len = MultiByteToWideChar(CP_UTF8, 0, weapon_class.c_str(), static_cast<int>(weapon_class.size()), nullptr, 0);
        if (len > 0) {
            std::wstring wname(len, 0);
            MultiByteToWideChar(CP_UTF8, 0, weapon_class.c_str(), static_cast<int>(weapon_class.size()), wname.data(), len);
            const std::wstring glyph = resolve_wide(wname);
            if (!glyph.empty())
                return wide_to_utf8(glyph);
        }
    }

    // Fallback by definition index
    if (def_index > 0) {
        auto it = kDefIndexToName.find(def_index);
        if (it != kDefIndexToName.end()) {
            const std::wstring glyph = resolve_wide(it->second);
            if (!glyph.empty())
                return wide_to_utf8(glyph);
        }
    }

    return {};
}

} // namespace weapon_icons
