#pragma once

#include <string>
#include <vector>

class ConfigStore {
public:
    static ConfigStore& instance();

    void startup();
    void refresh();
    bool save(const std::string& name);
    bool load(const std::string& name);
    bool remove(const std::string& name);
    void reset_defaults();

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
