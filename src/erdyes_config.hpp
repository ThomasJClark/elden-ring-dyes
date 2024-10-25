#pragma once

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace erdyes
{

struct color
{
    std::string name;
    std::wstring selected_message;
    std::wstring deselected_message;
    float red;
    float green;
    float blue;
};

/**
 * Load user preferences from an .ini file
 */
void load_config(const std::filesystem::path &ini_path);

namespace config
{
extern std::vector<color> colors;
};

};
