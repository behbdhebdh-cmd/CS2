#include "sdk/offset_update.hpp"
#include "sdk/offsets.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <winhttp.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <sstream>

#pragma comment(lib, "winhttp.lib")

namespace {

constexpr wchar_t kHost[] = L"www.cheatoffsets.com";
constexpr wchar_t kPath[] = L"/api/games/cs2/current";
constexpr DWORD kPollMs = 10 * 60 * 1000;

std::string lower_copy(std::string s)
{
    for (char& c : s)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

void skip_ws(const char*& p, const char* e)
{
    while (p < e && static_cast<unsigned char>(*p) <= 32)
        ++p;
}

bool parse_string(const char*& p, const char* e, std::string& out)
{
    skip_ws(p, e);
    if (p >= e || *p != '"')
        return false;
    ++p;
    out.clear();
    while (p < e && *p != '"') {
        if (*p == '\\' && p + 1 < e) {
            ++p;
            const char c = *p++;
            switch (c) {
            case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break;
            case 't': out.push_back('\t'); break;
            case '"': out.push_back('"'); break;
            case '\\': out.push_back('\\'); break;
            case '/': out.push_back('/'); break;
            case 'u':
                p = (std::min)(p + 4, e);
                out.push_back('?');
                break;
            default: out.push_back(c); break;
            }
        } else {
            out.push_back(*p++);
        }
    }
    if (p >= e)
        return false;
    ++p;
    return true;
}

bool skip_value(const char*& p, const char* e);

bool parse_object(const char*& p, const char* e, const std::string& prefix,
                  std::unordered_map<std::string, std::string>& kv, bool nested)
{
    skip_ws(p, e);
    if (p >= e || *p != '{')
        return false;
    ++p;
    skip_ws(p, e);
    if (p < e && *p == '}') {
        ++p;
        return true;
    }
    while (p < e) {
        std::string key;
        if (!parse_string(p, e, key))
            return false;
        skip_ws(p, e);
        if (p >= e || *p != ':')
            return false;
        ++p;
        skip_ws(p, e);
        if (p >= e)
            return false;

        const std::string dotted = prefix.empty() ? key : (prefix + "." + key);
        if (*p == '"') {
            std::string val;
            if (!parse_string(p, e, val))
                return false;
            kv[lower_copy(dotted)] = val;
        } else if (*p == '{' && nested) {
            if (!parse_object(p, e, dotted, kv, true))
                return false;
        } else {
            if (!skip_value(p, e))
                return false;
        }
        skip_ws(p, e);
        if (p < e && *p == ',') {
            ++p;
            continue;
        }
        if (p < e && *p == '}') {
            ++p;
            return true;
        }
        return false;
    }
    return false;
}

bool skip_value(const char*& p, const char* e)
{
    skip_ws(p, e);
    if (p >= e)
        return false;
    if (*p == '"') {
        std::string tmp;
        return parse_string(p, e, tmp);
    }
    if (*p == '{') {
        std::unordered_map<std::string, std::string> dump;
        return parse_object(p, e, {}, dump, true);
    }
    if (*p == '[') {
        ++p;
        skip_ws(p, e);
        if (p < e && *p == ']') {
            ++p;
            return true;
        }
        while (p < e) {
            if (!skip_value(p, e))
                return false;
            skip_ws(p, e);
            if (p < e && *p == ',') {
                ++p;
                continue;
            }
            if (p < e && *p == ']') {
                ++p;
                return true;
            }
            return false;
        }
        return false;
    }
    while (p < e && *p != ',' && *p != '}' && *p != ']' && static_cast<unsigned char>(*p) > 32)
        ++p;
    return true;
}

bool extract_object_by_key(const std::string& json, const char* key,
                           std::unordered_map<std::string, std::string>& kv, bool nested)
{
    const std::string needle = std::string("\"") + key + "\"";
    auto pos = json.find(needle);
    if (pos == std::string::npos)
        return false;
    const char* p = json.c_str() + pos + needle.size();
    const char* e = json.c_str() + json.size();
    skip_ws(p, e);
    if (p >= e || *p != ':')
        return false;
    ++p;
    return parse_object(p, e, {}, kv, nested);
}

bool extract_string_field(const std::string& json, const char* key, std::string& out)
{
    const std::string needle = std::string("\"") + key + "\"";
    auto pos = json.find(needle);
    if (pos == std::string::npos)
        return false;
    const char* p = json.c_str() + pos + needle.size();
    const char* e = json.c_str() + json.size();
    skip_ws(p, e);
    if (p >= e || *p != ':')
        return false;
    ++p;
    return parse_string(p, e, out);
}

bool parse_hex(const std::string& s, std::ptrdiff_t& out)
{
    const char* p = s.c_str();
    while (*p && static_cast<unsigned char>(*p) <= 32)
        ++p;
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X'))
        p += 2;
    if (!*p)
        return false;
    char* end = nullptr;
    const unsigned long long v = std::strtoull(p, &end, 16);
    if (!end || end == p)
        return false;
    out = static_cast<std::ptrdiff_t>(v);
    return true;
}

std::unordered_map<std::string, std::ptrdiff_t> to_numeric(
    const std::unordered_map<std::string, std::string>& kv)
{
    std::unordered_map<std::string, std::ptrdiff_t> out;
    out.reserve(kv.size());
    for (const auto& it : kv) {
        std::ptrdiff_t v = 0;
        if (parse_hex(it.second, v))
            out.emplace(it.first, v);
    }
    return out;
}

bool key_matches(const std::string& key, const char* spec)
{
    const std::string s = lower_copy(spec);
    if (key == s)
        return true;
    if (key.size() > s.size() && key[key.size() - s.size() - 1] == '.' &&
        key.compare(key.size() - s.size(), std::string::npos, s) == 0)
        return true;
    return false;
}

int apply_one(const std::unordered_map<std::string, std::ptrdiff_t>& flat,
              std::ptrdiff_t& dest, std::initializer_list<const char*> names)
{
    for (const char* n : names) {
        const std::string k = lower_copy(n);
        auto it = flat.find(k);
        if (it != flat.end()) {
            dest = it->second;
            return 1;
        }
    }
    for (const char* n : names) {
        for (const auto& kv : flat) {
            if (key_matches(kv.first, n)) {
                dest = kv.second;
                return 1;
            }
        }
    }
    return 0;
}

struct HttpResult {
    int status = 0;
    std::string etag;
    std::string body;
    std::string error;
};

std::string wide_to_utf8(const std::wstring& w)
{
    if (w.empty())
        return {};
    const int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
    std::string s(n, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), static_cast<int>(w.size()), s.data(), n, nullptr, nullptr);
    return s;
}

HttpResult http_get_current(const std::string& etag)
{
    HttpResult r;
    HINTERNET ses = WinHttpOpen(L"CS2OffsetPoll/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                                WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!ses) {
        r.error = "winhttp open failed";
        return r;
    }
    WinHttpSetTimeouts(ses, 8000, 8000, 10000, 10000);

    HINTERNET con = WinHttpConnect(ses, kHost, INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!con) {
        r.error = "connect failed";
        WinHttpCloseHandle(ses);
        return r;
    }

    HINTERNET req = WinHttpOpenRequest(con, L"GET", kPath, nullptr, WINHTTP_NO_REFERER,
                                       WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!req) {
        r.error = "open request failed";
        WinHttpCloseHandle(con);
        WinHttpCloseHandle(ses);
        return r;
    }

    std::wstring hdr = L"Accept: application/json\r\n";
    if (!etag.empty()) {
        std::wstring wetag(etag.begin(), etag.end());
        hdr += L"If-None-Match: " + wetag + L"\r\n";
    }
    WinHttpAddRequestHeaders(req, hdr.c_str(), static_cast<DWORD>(-1), WINHTTP_ADDREQ_FLAG_ADD);

    if (!WinHttpSendRequest(req, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !WinHttpReceiveResponse(req, nullptr)) {
        r.error = "request failed";
        WinHttpCloseHandle(req);
        WinHttpCloseHandle(con);
        WinHttpCloseHandle(ses);
        return r;
    }

    DWORD status = 0, slen = sizeof(status);
    WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &status, &slen, WINHTTP_NO_HEADER_INDEX);
    r.status = static_cast<int>(status);

    DWORD esz = 0;
    WinHttpQueryHeaders(req, WINHTTP_QUERY_ETAG, WINHTTP_HEADER_NAME_BY_INDEX, nullptr, &esz, WINHTTP_NO_HEADER_INDEX);
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER && esz) {
        std::wstring wetag((esz / sizeof(wchar_t)) + 1, 0);
        if (WinHttpQueryHeaders(req, WINHTTP_QUERY_ETAG, WINHTTP_HEADER_NAME_BY_INDEX,
                                wetag.data(), &esz, WINHTTP_NO_HEADER_INDEX)) {
            wetag.resize(esz / sizeof(wchar_t));
            while (!wetag.empty() && (wetag.back() == 0 || wetag.back() == L'\r' || wetag.back() == L'\n'))
                wetag.pop_back();
            r.etag = wide_to_utf8(wetag);
        }
    }

    if (r.status == 200) {
        for (;;) {
            DWORD avail = 0;
            if (!WinHttpQueryDataAvailable(req, &avail) || !avail)
                break;
            std::string chunk(avail, 0);
            DWORD got = 0;
            if (!WinHttpReadData(req, chunk.data(), avail, &got) || !got)
                break;
            chunk.resize(got);
            r.body.append(chunk);
        }
    }

    WinHttpCloseHandle(req);
    WinHttpCloseHandle(con);
    WinHttpCloseHandle(ses);
    return r;
}

std::string exe_dir()
{
    char buf[MAX_PATH]{};
    const DWORD n = GetModuleFileNameA(nullptr, buf, MAX_PATH);
    if (!n)
        return {};
    std::string p(buf, buf + n);
    const auto slash = p.find_last_of("\\/");
    return slash == std::string::npos ? p : p.substr(0, slash);
}

void set_meta(char* buf, size_t cap, const std::string& v)
{
    if (!buf || !cap)
        return;
    std::snprintf(buf, cap, "%s", v.empty() ? "?" : v.c_str());
}

} // namespace

OffsetUpdate& OffsetUpdate::instance()
{
    static OffsetUpdate g;
    return g;
}

OffsetUpdate::~OffsetUpdate()
{
    stop();
}

void OffsetUpdate::start()
{
    if (running_.exchange(true))
        return;
    load_cache();
    stop_event_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    thread_ = std::thread([this] { worker(); });
}

void OffsetUpdate::stop()
{
    if (!running_.exchange(false)) {
        if (thread_.joinable())
            thread_.join();
        return;
    }
    if (stop_event_)
        SetEvent(static_cast<HANDLE>(stop_event_));
    if (thread_.joinable())
        thread_.join();
    if (stop_event_) {
        CloseHandle(static_cast<HANDLE>(stop_event_));
        stop_event_ = nullptr;
    }
}

void OffsetUpdate::request_poll()
{
    force_poll_.store(true, std::memory_order_release);
    if (stop_event_)
        SetEvent(static_cast<HANDLE>(stop_event_));
}

void OffsetUpdate::tick()
{
    std::unordered_map<std::string, std::ptrdiff_t> flat;
    std::string ver, upd;
    {
        std::lock_guard<std::mutex> lock(mu_);
        if (!pending_ready_)
            return;
        flat = std::move(pending_);
        ver = std::move(pending_version_);
        upd = std::move(pending_updated_);
        pending_ready_ = false;
        pending_.clear();
    }
    apply_flat(flat, ver, upd);
}

std::string OffsetUpdate::status() const
{
    std::lock_guard<std::mutex> lock(mu_);
    return status_;
}

std::string OffsetUpdate::cache_path() const
{
    const std::string cands[] = {
        "D:\\CS2\\config\\offsets_cache.json",
        exe_dir() + "\\..\\config\\offsets_cache.json",
        exe_dir() + "\\config\\offsets_cache.json",
        "config\\offsets_cache.json",
    };
    for (const auto& p : cands) {
        const auto slash = p.find_last_of("\\/");
        if (slash == std::string::npos)
            continue;
        const std::string dir = p.substr(0, slash);
        const DWORD a = GetFileAttributesA(dir.c_str());
        if (a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY))
            return p;
    }
    return "D:\\CS2\\config\\offsets_cache.json";
}

void OffsetUpdate::load_cache()
{
    std::ifstream in(cache_path(), std::ios::binary);
    if (!in)
        return;
    std::ostringstream ss;
    ss << in.rdbuf();
    const std::string json = ss.str();
    if (json.size() < 8)
        return;

    extract_string_field(json, "etag", etag_);
    std::string ver, upd;
    extract_string_field(json, "version", ver);
    extract_string_field(json, "updated_at", upd);

    std::unordered_map<std::string, std::string> kv;
    if (!extract_object_by_key(json, "offsets_flat", kv, false))
        return;
    const auto flat = to_numeric(kv);
    if (flat.empty())
        return;
    apply_flat(flat, ver, upd);
    live_.store(true, std::memory_order_release);
    std::lock_guard<std::mutex> lock(mu_);
    status_ = "cache · " + (ver.empty() ? std::string("unknown") : ver);
}

void OffsetUpdate::apply_flat(const std::unordered_map<std::string, std::ptrdiff_t>& flat,
                              const std::string& version, const std::string& updated)
{
    int n = 0;
    n += apply_one(flat, offsets::client::dwCSGOInput, { "client_dll.dwcsgoinput", "dwcsgoinput" });
    n += apply_one(flat, offsets::client::dwEntityList, { "client_dll.dwentitylist", "dwentitylist" });
    n += apply_one(flat, offsets::client::dwGameEntitySystem, { "client_dll.dwgameentitysystem", "dwgameentitysystem" });
    n += apply_one(flat, offsets::client::dwGameEntitySystem_highestEntityIndex,
                   { "client_dll.dwgameentitysystem_highestentityindex", "dwgameentitysystem_highestentityindex" });
    n += apply_one(flat, offsets::client::dwGameRules, { "client_dll.dwgamerules", "dwgamerules" });
    n += apply_one(flat, offsets::client::dwGlobalVars, { "client_dll.dwglobalvars", "dwglobalvars" });
    n += apply_one(flat, offsets::client::dwGlowManager, { "client_dll.dwglowmanager", "dwglowmanager" });
    n += apply_one(flat, offsets::client::dwLocalPlayerController, { "client_dll.dwlocalplayercontroller", "dwlocalplayercontroller" });
    n += apply_one(flat, offsets::client::dwLocalPlayerPawn, { "client_dll.dwlocalplayerpawn", "dwlocalplayerpawn" });
    n += apply_one(flat, offsets::client::dwPlantedC4, { "client_dll.dwplantedc4", "dwplantedc4" });
    n += apply_one(flat, offsets::client::dwPrediction, { "client_dll.dwprediction", "dwprediction" });
    n += apply_one(flat, offsets::client::dwSensitivity, { "client_dll.dwsensitivity", "dwsensitivity" });
    n += apply_one(flat, offsets::client::dwSensitivity_sensitivity, { "client_dll.dwsensitivity_sensitivity", "dwsensitivity_sensitivity" });
    n += apply_one(flat, offsets::client::dwViewAngles, { "client_dll.dwviewangles", "dwviewangles" });
    n += apply_one(flat, offsets::client::dwViewMatrix, { "client_dll.dwviewmatrix", "dwviewmatrix" });
    n += apply_one(flat, offsets::client::dwViewRender, { "client_dll.dwviewrender", "dwviewrender" });
    n += apply_one(flat, offsets::client::dwWeaponC4, { "client_dll.dwweaponc4", "dwweaponc4" });

    n += apply_one(flat, offsets::engine2::dwBuildNumber, { "engine2_dll.dwbuildnumber", "dwbuildnumber" });
    n += apply_one(flat, offsets::engine2::dwNetworkGameClient, { "engine2_dll.dwnetworkgameclient", "dwnetworkgameclient" });
    n += apply_one(flat, offsets::engine2::dwNetworkGameClient_clientTickCount,
                   { "engine2_dll.dwnetworkgameclient_clienttickcount", "dwnetworkgameclient_clienttickcount" });
    n += apply_one(flat, offsets::engine2::dwNetworkGameClient_signOnState,
                   { "engine2_dll.dwnetworkgameclient_signonstate", "dwnetworkgameclient_signonstate" });
    n += apply_one(flat, offsets::engine2::dwNetworkGameClient_localPlayer,
                   { "engine2_dll.dwnetworkgameclient_localplayer", "dwnetworkgameclient_localplayer" });
    n += apply_one(flat, offsets::engine2::dwNetworkGameClient_maxClients,
                   { "engine2_dll.dwnetworkgameclient_maxclients", "dwnetworkgameclient_maxclients" });
    n += apply_one(flat, offsets::engine2::dwWindowHeight, { "engine2_dll.dwwindowheight", "dwwindowheight" });
    n += apply_one(flat, offsets::engine2::dwWindowWidth, { "engine2_dll.dwwindowwidth", "dwwindowwidth" });

    n += apply_one(flat, offsets::schema::C_BaseEntity::m_pGameSceneNode, { "c_baseentity.m_pgamescenenode", "m_pgamescenenode" });
    n += apply_one(flat, offsets::schema::C_BaseEntity::m_pCollision, { "c_baseentity.m_pcollision", "m_pcollision" });
    n += apply_one(flat, offsets::schema::C_BaseEntity::m_iMaxHealth, { "c_baseentity.m_imaxhealth", "m_imaxhealth" });
    n += apply_one(flat, offsets::schema::C_BaseEntity::m_iHealth, { "c_baseentity.m_ihealth", "m_ihealth" });
    n += apply_one(flat, offsets::schema::C_BaseEntity::m_lifeState, { "c_baseentity.m_lifestate", "m_lifestate" });
    n += apply_one(flat, offsets::schema::C_BaseEntity::m_iTeamNum, { "c_baseentity.m_iteamnum", "m_iteamnum" });
    n += apply_one(flat, offsets::schema::C_BaseEntity::m_fFlags, { "c_baseentity.m_fflags", "m_fflags" });
    n += apply_one(flat, offsets::schema::C_BaseEntity::m_vecAbsVelocity, { "c_baseentity.m_vecabsvelocity", "m_vecabsvelocity" });
    n += apply_one(flat, offsets::schema::C_BaseEntity::m_hOwnerEntity, { "c_baseentity.m_hownerentity", "m_hownerentity" });

    n += apply_one(flat, offsets::schema::CGameSceneNode::m_vecAbsOrigin, { "cgamescenenode.m_vecabsorigin", "m_vecabsorigin" });
    n += apply_one(flat, offsets::schema::CGameSceneNode::m_bDormant, { "cgamescenenode.m_bdormant", "m_bdormant" });

    n += apply_one(flat, offsets::schema::C_BaseModelEntity::m_Collision, { "c_basemodelentity.m_collision", "c_basemodelentity.m_collision" });
    n += apply_one(flat, offsets::schema::C_BaseModelEntity::m_vecViewOffset, { "c_basemodelentity.m_vecviewoffset", "m_vecviewoffset" });

    n += apply_one(flat, offsets::schema::CCollisionProperty::m_vecMins, { "ccollisionproperty.m_vecmins", "m_vecmins" });
    n += apply_one(flat, offsets::schema::CCollisionProperty::m_vecMaxs, { "ccollisionproperty.m_vecmaxs", "m_vecmaxs" });

    n += apply_one(flat, offsets::schema::C_BasePlayerPawn::m_vOldOrigin, { "c_baseplayerpawn.m_voldorigin", "m_voldorigin" });
    n += apply_one(flat, offsets::schema::C_BasePlayerPawn::m_hController, { "c_baseplayerpawn.m_hcontroller", "m_hcontroller" });

    n += apply_one(flat, offsets::schema::CBasePlayerController::m_hPawn, { "cbaseplayercontroller.m_hpawn", "m_hpawn" });
    n += apply_one(flat, offsets::schema::CBasePlayerController::m_iszPlayerName, { "cbaseplayercontroller.m_iszplayername", "m_iszplayername" });
    n += apply_one(flat, offsets::schema::CBasePlayerController::m_steamID, { "cbaseplayercontroller.m_steamid", "m_steamid" });
    n += apply_one(flat, offsets::schema::CBasePlayerController::m_bIsLocalPlayerController,
                   { "cbaseplayercontroller.m_bislocalplayercontroller", "m_bislocalplayercontroller" });

    n += apply_one(flat, offsets::schema::CCSPlayerController::m_sSanitizedPlayerName,
                   { "ccsplayercontroller.m_ssanitizedplayername", "m_ssanitizedplayername" });
    n += apply_one(flat, offsets::schema::CCSPlayerController::m_hPlayerPawn, { "ccsplayercontroller.m_hplayerpawn", "m_hplayerpawn" });
    n += apply_one(flat, offsets::schema::CCSPlayerController::m_bPawnIsAlive, { "ccsplayercontroller.m_bpawnisalive", "m_bpawnisalive" });
    n += apply_one(flat, offsets::schema::CCSPlayerController::m_iPawnHealth, { "ccsplayercontroller.m_ipawnhealth", "m_ipawnhealth" });
    n += apply_one(flat, offsets::schema::CCSPlayerController::m_iPing, { "ccsplayercontroller.m_iping", "m_iping" });
    n += apply_one(flat, offsets::schema::CCSPlayerController::m_iPawnArmor, { "ccsplayercontroller.m_ipawnarmor", "m_ipawnarmor" });

    if (!version.empty())
        set_meta(offsets::kDumpUpdate, sizeof(offsets::kDumpUpdate), version);
    if (!updated.empty())
        set_meta(offsets::kDumpUtc, sizeof(offsets::kDumpUtc), updated);
    set_meta(offsets::kDumpSource, sizeof(offsets::kDumpSource), n ? "api" : "baked");

    std::lock_guard<std::mutex> lock(mu_);
    char line[160];
    std::snprintf(line, sizeof(line), "live · %s · %d fields",
                  version.empty() ? offsets::kDumpUpdate : version.c_str(), n);
    status_ = line;
    live_.store(n > 0, std::memory_order_release);
}

void OffsetUpdate::poll_once()
{
    std::string etag;
    {
        std::lock_guard<std::mutex> lock(mu_);
        etag = etag_;
        status_ = "polling · cheatoffsets.com";
    }

    const HttpResult r = http_get_current(etag);
    if (r.status == 304) {
        std::lock_guard<std::mutex> lock(mu_);
        status_ = std::string("etag 304 · ") + offsets::kDumpUpdate + " unchanged";
        return;
    }
    if (r.status != 200 || r.body.empty()) {
        std::lock_guard<std::mutex> lock(mu_);
        if (!r.error.empty())
            status_ = "error · " + r.error;
        else
            status_ = "error · HTTP " + std::to_string(r.status ? r.status : 0);
        return;
    }

    std::unordered_map<std::string, std::string> kv;
    if (!extract_object_by_key(r.body, "offsets_flat", kv, false)) {
        extract_object_by_key(r.body, "offsets", kv, true);
    }
    auto flat = to_numeric(kv);
    if (flat.empty()) {
        std::lock_guard<std::mutex> lock(mu_);
        status_ = "error · no offsets_flat";
        return;
    }

    std::string ver, upd;
    extract_string_field(r.body, "version", ver);
    if (ver.empty())
        extract_string_field(r.body, "label", ver);
    extract_string_field(r.body, "updated_at", upd);

    {
        std::lock_guard<std::mutex> lock(mu_);
        if (!r.etag.empty())
            etag_ = r.etag;
        pending_ = std::move(flat);
        pending_version_ = ver;
        pending_updated_ = upd;
        pending_ready_ = true;
        status_ = "fetched · applying";
    }

    // Persist the original API body plus etag for the next If-None-Match.
    {
        const std::string path = cache_path();
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        if (out) {
            out << "{\n  \"etag\": \"";
            for (char c : r.etag) {
                if (c == '"' || c == '\\') out << '\\';
                out << c;
            }
            out << "\",\n  \"version\": \"";
            for (char c : ver) {
                if (c == '"' || c == '\\') out << '\\';
                out << c;
            }
            out << "\",\n  \"updated_at\": \"";
            for (char c : upd) {
                if (c == '"' || c == '\\') out << '\\';
                out << c;
            }
            out << "\",\n  \"offsets_flat\": {";
            bool first = true;
            std::unordered_map<std::string, std::string> raw;
            extract_object_by_key(r.body, "offsets_flat", raw, false);
            for (const auto& it : raw) {
                if (!first) out << ',';
                first = false;
                out << "\n    \"" << it.first << "\": \"";
                for (char c : it.second) {
                    if (c == '"' || c == '\\') out << '\\';
                    out << c;
                }
                out << '"';
            }
            out << "\n  }\n}\n";
        }
    }
}

void OffsetUpdate::worker()
{
    auto ev = static_cast<HANDLE>(stop_event_);
    while (running_.load(std::memory_order_acquire)) {
        force_poll_.store(false, std::memory_order_release);
        poll_once();
        if (!running_.load(std::memory_order_acquire))
            break;
        if (!ev) {
            Sleep(kPollMs);
            continue;
        }
        ResetEvent(ev);
        WaitForSingleObject(ev, kPollMs);
    }
}
