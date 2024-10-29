#pragma once

#include <filesystem>

namespace erdyes
{

/**
 * Load user preferences from an .ini file
 */
void load_config(const std::filesystem::path &ini_path);

};
