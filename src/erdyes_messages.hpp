#pragma once

#include <map>
#include <string>

namespace erdyes
{

void setup_messages();

struct messages_type
{
    std::wstring apply_dyes;
    std::wstring primary_color;
    std::wstring secondary_color;
    std::wstring tertiary_color;
    std::wstring primary_intensity;
    std::wstring secondary_intensity;
    std::wstring tertiary_intensity;
    std::wstring none;
    std::wstring back;
};

extern messages_type messages;
extern std::map<std::string, messages_type> messages_by_lang;

/**
 * Format one of the color or intensity menu options
 */
std::wstring format_option_message(std::wstring const &label, bool selected, bool rtl = false);

// Message IDs in EventTextForTalk.fmg
namespace event_text_for_talk
{
static constexpr int cancel = 15000372;
static constexpr int sort_chest = 15000395;

static constexpr int mod_message_start = 670000000;
static constexpr int mod_message_end = 670100000;

static constexpr int apply_dyes = 670000000;
static constexpr int primary_color = 670000003;
static constexpr int secondary_color = 670000004;
static constexpr int tertiary_color = 670000005;
static constexpr int primary_intensity = 670000006;
static constexpr int secondary_intensity = 670000007;
static constexpr int tertiary_intensity = 670000008;
static constexpr int none_deselected = 670000009;
static constexpr int none_selected = 670000010;
static constexpr int back = 670000099;
static constexpr int dye_intensity_selected_start = 670010000;
static constexpr int dye_intensity_deselected_start = 670020000;
static constexpr int dye_color_selected_start = 670030000;
static constexpr int dye_color_deselected_start = 670040000;
}

}
