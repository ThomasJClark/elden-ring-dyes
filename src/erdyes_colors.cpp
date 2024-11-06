#include "erdyes_colors.hpp"
#include "erdyes_talkscript.hpp"

#include <elden-x/chr/world_chr_man.hpp>
#include <elden-x/utils/modutils.hpp>
#include <spdlog/spdlog.h>
#include <string>
#include <thread>

std::vector<erdyes::color> erdyes::colors;
std::vector<erdyes::intensity> erdyes::intensities;

int erdyes::primary_color_index = -1;
int erdyes::secondary_color_index = -1;
int erdyes::tertiary_color_index = -1;
int erdyes::primary_intensity_index = 2;
int erdyes::secondary_intensity_index = 2;
int erdyes::tertiary_intensity_index = 2;

static const std::wstring primary_color_material_ex_name = L"[Albedo]_1_[Tint]";
static const std::wstring secondary_color_material_ex_name = L"[Albedo]_3_[Tint]";
static const std::wstring tertiary_color_material_ex_name = L"[Albedo]_4_[Tint]";

static void update_colors(from::CS::ChrIns *);

static from::CS::WorldChrManImp **world_chr_man_addr;

// CS::ChrIns::Update(float delta_time)
static void (*chr_ins_update)(from::CS::ChrIns *, float);
static void chr_ins_update_detour(from::CS::ChrIns *_this, float delta_time)
{
    chr_ins_update(_this, delta_time);

    if (_this == (*world_chr_man_addr)->main_player)
    {
        update_colors(_this);
    }
}

static bool is_valid_color_index(int index)
{
    return index >= 0 && index < erdyes::colors.size();
}

static bool is_valid_intensity_index(int index)
{
    return index >= 0 && index < erdyes::intensities.size();
}

/**
 * Upsert a single material parameter by name using one of the configured color options
 */
static void update_color(from::CS::ChrIns *chr, const std::wstring &name, erdyes::color &color,
                         erdyes::intensity &intensity, float alpha = 1.0f)
{
    auto new_modifier = from::CS::CSChrModelParamModifierModule::modifier{
        .name = name.data(),
        .value = {.material_id = 1,
                  .value1 = color.red,
                  .value2 = color.green,
                  .value3 = color.blue,
                  .value4 = alpha,
                  .value5 = intensity.intensity},
    };

    auto &modifiers = chr->modules->model_param_modifier_module->modifiers;

    // Update the modifier if it's already applied to the character, otherwise insert it
    // into the vector. Note that we write over the entire modifier and not just the RGB
    // fields because the game zeros out this memory each frame.
    for (auto &modifier : modifiers)
    {
        if (modifier.name == name)
        {
            modifier = new_modifier;
            return;
        }
    }

    modifiers.push_back(new_modifier);
};

/**
 * Updates material parameters for the given character based on the selected colors
 */
static void update_colors(from::CS::ChrIns *chr)
{
    auto primary_color_index = erdyes::primary_color_index;
    auto secondary_color_index = erdyes::secondary_color_index;
    auto tertiary_color_index = erdyes::tertiary_color_index;
    auto primary_intensity_index = erdyes::primary_intensity_index;
    auto secondary_intensity_index = erdyes::secondary_intensity_index;
    auto tertiary_intensity_index = erdyes::tertiary_intensity_index;

    // Override the selected index with the focused dialog option, so you can hover over menu
    // options to preview them.
    auto focused_entry = erdyes::talkscript::get_focused_entry();
    switch (erdyes::talkscript::dye_target)
    {
    case erdyes::talkscript::dye_target_type::primary_color:
        // The minus 1 accounts for the "none" option in the color menu
        if (is_valid_color_index(focused_entry - 1))
            primary_color_index = focused_entry - 1;
        break;
    case erdyes::talkscript::dye_target_type::secondary_color:
        if (is_valid_color_index(focused_entry - 1))
            secondary_color_index = focused_entry - 1;
        break;
    case erdyes::talkscript::dye_target_type::tertiary_color:
        if (is_valid_color_index(focused_entry - 1))
            tertiary_color_index = focused_entry - 1;
        break;
    case erdyes::talkscript::dye_target_type::primary_intensity:
        if (is_valid_intensity_index(focused_entry))
            primary_intensity_index = focused_entry;
        break;
    case erdyes::talkscript::dye_target_type::secondary_intensity:
        if (is_valid_intensity_index(focused_entry))
            secondary_intensity_index = focused_entry;
        break;
    case erdyes::talkscript::dye_target_type::tertiary_intensity:
        if (is_valid_intensity_index(focused_entry))
            tertiary_intensity_index = focused_entry;
        break;
    }

    // Update the player's material modifiers to have the RGB and intensity values of the chosen
    // colors
    if (is_valid_color_index(primary_color_index))
    {
        update_color(chr, primary_color_material_ex_name, erdyes::colors[primary_color_index],
                     erdyes::intensities[primary_intensity_index]);
    }
    if (is_valid_color_index(secondary_color_index))
    {
        update_color(chr, secondary_color_material_ex_name, erdyes::colors[secondary_color_index],
                     erdyes::intensities[secondary_intensity_index]);
    }
    if (is_valid_color_index(tertiary_color_index))
    {
        update_color(chr, tertiary_color_material_ex_name, erdyes::colors[tertiary_color_index],
                     erdyes::intensities[tertiary_intensity_index]);
    }
}

void erdyes::apply_colors_init()
{
    world_chr_man_addr = modutils::scan<from::CS::WorldChrManImp *>({
        .aob = "48 8b 05 ?? ?? ?? ??"  // mov rax, [WorldChrMan]
               "48 85 c0"              // test rax, rax
               "74 0f"                 // jz end_label
               "48 39 88 08 e5 01 00", // cmp [rax + 0x1e508], rcx
        .relative_offsets = {{3, 7}},
    });

    modutils::hook(
        {
            .aob = "84 c0"                          // test al, al
                   "74 09"                          // je 11
                   "c6 87 ?? ?? ?? ?? 01"           // mov byte ptr [rdi + ????], 1
                   "eb 0a"                          // jmp 21
                   "c7 87 ?? ?? ?? ?? 00 00 00 00", // mov dword ptr [rdi + ????], 0
            .offset = -203,
        },
        chr_ins_update_detour, chr_ins_update);
}
