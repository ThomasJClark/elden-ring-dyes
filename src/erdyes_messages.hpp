#pragma once

namespace erdyes
{

void setup_messages();

// Message IDs in EventTextForTalk.fmg
namespace event_text_for_talk
{
static constexpr int cancel = 15000372;
static constexpr int sort_chest = 15000395;

static constexpr int mod_message_start = 67000000;
static constexpr int mod_message_end = 67100000;

static constexpr int apply_dyes = 67000000;
static constexpr int primary_color = 67000003;
static constexpr int secondary_color = 67000004;
static constexpr int tertiary_color = 67000005;
static constexpr int primary_intensity = 67000006;
static constexpr int secondary_intensity = 67000007;
static constexpr int tertiary_intensity = 67000008;
static constexpr int none_deselected = 67000009;
static constexpr int none_selected = 67000010;
static constexpr int back = 67000099;
static constexpr int dye_intensity_selected_start = 67010000;
static constexpr int dye_intensity_deselected_start = 67020000;
static constexpr int dye_color_selected_start = 67030000;
static constexpr int dye_color_deselected_start = 67040000;
}

}
