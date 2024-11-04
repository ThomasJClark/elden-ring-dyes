#define MINI_CASE_SENSITIVE

#include "erdyes_config.hpp"
#include "erdyes_state.hpp"

#include <codecvt>
#include <locale>
#include <mini/ini.h>
#include <spdlog/spdlog.h>

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

constexpr auto selected_img_src = L"MENU_Lockon_01a.png";
constexpr auto deselected_img_src = L"MENU_DummyTransparent.dds";

static constexpr std::wstring format_message(std::wstring const &hex_code, std::wstring const &name,
                                             bool selected)
{
    auto img_src = selected ? selected_img_src : deselected_img_src;
    return std::wstring{L"<IMG SRC='img://"} + img_src +
           L"' WIDTH='20' HEIGHT='20' HSPACE='0' VSPACE='-1'>" +
           L"<FONT FACE='Bingus Sans' COLOR='" + hex_code + L"'>" + L"*</FONT> " + name;
};

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
        auto &colors_ini = ini["colors"];

        auto converter = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>{};

        erdyes::state::colors.reserve(colors_ini.size());
        for (auto &[name, hex_code] : ini["colors"])
        {
            int elements[3];
            if (parse_hex_code(hex_code, elements))
            {
                std::wstring name_l = converter.from_bytes(name);
                std::wstring hex_code_l = converter.from_bytes(hex_code);

                erdyes::state::colors.emplace_back(format_message(hex_code_l, name_l, true),
                                                   format_message(hex_code_l, name_l, false),
                                                   elements[0] / 255.0f, elements[1] / 255.0f,
                                                   elements[2] / 255.0f);

                spdlog::info("Added color definition \"{} = {}\"", name, hex_code);
            }
            else
            {
                spdlog::error("Invalid color definition \"{} = {}\"", name, hex_code);
            }
        }

        spdlog::info("Added {} colors", erdyes::state::colors.size());

        erdyes::state::intensities.emplace_back(format_message(L"#111111", L"Faint", true),
                                                format_message(L"#111111", L"Faint", false), 0.25f);
        erdyes::state::intensities.emplace_back(format_message(L"#444444", L"Low", true),
                                                format_message(L"#444444", L"Low", false), 0.5f);
        erdyes::state::intensities.emplace_back(format_message(L"#777777", L"Normal", true),
                                                format_message(L"#777777", L"Normal", false), 1.0f);
        erdyes::state::intensities.emplace_back(format_message(L"#bbbbbb", L"High", true),
                                                format_message(L"#bbbbbb", L"High", false), 2.5f);
        erdyes::state::intensities.emplace_back(format_message(L"#ffffff", L"Extreme", true),
                                                format_message(L"#ffffff", L"Extreme", false),
                                                10.0f);
    }
}
