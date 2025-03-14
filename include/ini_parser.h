#pragma once

#include <string>
#include <map>
#include <vector>

class IniParser {
public:
    bool load_file(const std::string& filename);
    bool has_section(const std::string& section) const;
    std::string get_string(const std::string& section, const std::string& key, const std::string& default_value = "") const;
    bool get_bool(const std::string& section, const std::string& key, bool default_value = false) const;
    int get_int(const std::string& section, const std::string& key, int default_value = 0) const;

private:
    std::map<std::string, std::map<std::string, std::string>> sections_;
    std::string current_section_;
    
    void parse_line(const std::string& line);
    std::string trim(const std::string& str) const;
}; 