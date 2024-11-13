#define MINI_CASE_SENSITIVE

#include "erdyes_config.hpp"
#include "erdyes_state.hpp"

#include <codecvt>
#include <locale>
#include <mini/ini.h>
#include <spdlog/spdlog.h>

#ifdef _DEBUG
bool erdyes::config::debug = true;
#else
bool erdyes::config::debug = false;
#endif

unsigned int erdyes::config::initialize_delay = 5000;

/**
 * Parse an HTML-style hexadecimal color code, returning true if successful
 */
static bool parse_hex_code(const std::string &str, int elements[3])
{
    if (str[0] != '#' || (str.size() != 4 && str.size() != 7))
    {
        return false;
    }

    int digits[6];
    for (int i = 1; i < str.size(); ++i)
    {
        auto chr = str[i];
        auto &digit = digits[i - 1];

        if (chr >= '0' && chr <= '9')
            digit = chr - '0';
        else if (chr >= 'A' && chr <= 'F')
            digit = 0xa + chr - 'A';
        else if (chr >= 'a' && chr <= 'f')
            digit = 0xa + chr - 'a';
        else
            return false;
    }

    if (str.size() == 4)
    {
        elements[0] = digits[0] * 0x11;
        elements[1] = digits[1] * 0x11;
        elements[2] = digits[2] * 0x11;
    }
    else
    {
        elements[0] = digits[0] * 0x10 + digits[1];
        elements[1] = digits[2] * 0x10 + digits[3];
        elements[2] = digits[4] * 0x10 + digits[5];
    }

    return true;
}

void erdyes::load_config(const std::filesystem::path &ini_path)
{
    spdlog::info("Loading config from {}", ini_path.string());

    mINI::INIFile file(ini_path.string());

    mINI::INIStructure ini;
    if (!file.read(ini))
    {
        spdlog::warn("Failed to read config");
        return;
    }

    if (ini.has("erdyes"))
    {
        auto &erdyes_config = ini["erdyes"];

        if (erdyes_config.has("debug") && erdyes_config["debug"] != "false")
        {
            erdyes::config::debug = true;
        }

        if (erdyes_config.has("initialize_delay"))
            erdyes::config::initialize_delay =
                std::stoi(erdyes_config["initialize_delay"], nullptr, 10);
    }

    if (ini.has("colors"))
    {
        auto &colors_config = ini["colors"];

        auto converter = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>{};

        erdyes::colors.reserve(colors_config.size());
        for (auto &[name, hex_code] : colors_config)
        {
            int elements[3];
            if (parse_hex_code(hex_code, elements))
            {
                erdyes::add_color_option(converter.from_bytes(name), converter.from_bytes(hex_code),
                                         elements[0] / 255.0f, elements[1] / 255.0f,
                                         elements[2] / 255.0f);

                spdlog::info("Added color definition \"{} = {}\"", name, hex_code);
            }
            else
            {
                spdlog::error("Invalid color definition \"{} = {}\"", name, hex_code);
            }
        }

        spdlog::info("Added {} colors", erdyes::colors.size());
    }
}
