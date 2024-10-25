#define MINI_CASE_SENSITIVE

#include "erdyes_config.hpp"

#include <mini/ini.h>
#include <spdlog/spdlog.h>

std::vector<erdyes::color> erdyes::config::colors;

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

    if (ini.has("colors"))
    {
        auto colors_ini = ini["colors"];

        config::colors.reserve(colors_ini.size());
        for (auto &[name, hex_code] : ini["colors"])
        {
            int elements[3];
            if (parse_hex_code(hex_code, elements))
            {
                // TODO decode UTF-8
                std::wstring name_l = {name.begin(), name.end()};
                std::wstring hex_code_l = {hex_code.begin(), hex_code.end()};

                auto format_message = [&name_l, &hex_code_l](std::wstring const &img_src) -> auto {
                    return L"<IMG SRC='img://" + img_src + L"' WIDTH='20' HEIGHT='20' HSPACE='0' " +
                           L"VSPACE='-1'><FONT FACE='Bingus Sans' COLOR='" + hex_code_l + L"'>" +
                           L"*</FONT> " + name_l;
                };

                config::colors.emplace_back(name, format_message(L"MENU_Lockon_01a.png"),
                                            format_message(L"MENU_DummyTransparent.dds"),
                                            elements[0] / 255.0f, elements[1] / 255.0f,
                                            elements[2] / 255.0f);

                spdlog::info("Added color definition \"{} = {}\"", name, hex_code);
            }
            else
            {
                spdlog::error("Invalid color definition \"{} = {}\"", name, hex_code);
            }
        }

        spdlog::info("Added {} colors", erdyes::config::colors.size());
    }
}
