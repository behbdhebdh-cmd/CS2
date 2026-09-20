#pragma once

#include <string>
#include <vector>

struct HWND__; typedef HWND__* HWND;

class ConfigStore {
public:
    static ConfigStore& instance();

    void startup();
    void refresh();
    bool save(const std::string& name);
    bool load(const std::string& name);
    bool remove(const std::string& name);
    // Freie Pfade (Save As / Open): akzeptieren .json und .cfg.
    bool save_to_file(const std::string& path);
    bool load_from_file(const std::string& path);
    // Native Explorer-Dialoge (modal zum Overlay). false = abgebrochen.
    bool save_as_dialog(HWND owner, const std::string& initial_name);
    bool open_dialog(HWND owner);
    void reset_defaults();
    bool apply_preset(int id);
    static int preset_count() { return 3; }
    static const char* preset_label(int id);

    const std::vector<std::string>& names() const { return names_; }
    const std::string& status() const { return status_; }
    const std::string& dir() const { return dir_; }
    const std::string& last_loaded() const { return last_loaded_; }

private:
    ConfigStore() = default;
    void ensure_dir();
    std::string path_for(const std::string& name) const;
    void log(const char* fmt, ...);

    std::string dir_ = "D:\\CS2\\configs";
    std::string status_ = "idle";
    std::string last_loaded_;
    std::vector<std::string> names_;
};

std::string sanitize_config_name(const std::string& raw);
