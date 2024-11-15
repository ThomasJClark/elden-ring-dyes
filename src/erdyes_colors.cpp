/**
 * erdyes_colors.hpp
 *
 * Applies colors to the player character based on the current selections
 */
#include "erdyes_colors.hpp"
#include "erdyes_state.hpp"
#include "erdyes_talkscript.hpp"

#include <elden-x/chr/world_chr_man.hpp>
#include <elden-x/utils/modutils.hpp>
#include <spdlog/spdlog.h>
#include <string>

static const std::wstring albedo1_material_ex_name = L"[Albedo]_1_[Tint]";
static const std::wstring albedo2_material_ex_name = L"[Albedo]_2_[Tint]";
static const std::wstring albedo3_material_ex_name = L"[Albedo]_3_[Tint]";
static const std::wstring albedo4_material_ex_name = L"[Albedo]_4_[Tint]";

static const int player_copy_sentinel = 0;

static void update_main_player_colors(from::CS::ChrIns *);

// CS::PlayerIns::Update(float delta_time)
static void (*cs_player_update)(from::CS::PlayerIns *, float);
static void cs_player_update_detour(from::CS::PlayerIns *_this, float delta_time)
{
    cs_player_update(_this, delta_time);

    auto &chr_asm = _this->player_game_data->equip_game_data.chr_asm;

    // Update the main player's colors based on the items in their inventory
    if (_this == from::CS::WorldChrManImp::instance()->main_player)
    {
        update_main_player_colors(_this);
    }
    // Apply the main player logic to any mimic tears as well
    else if (chr_asm.gear_param_ids.unused4 == player_copy_sentinel)
    {
        update_main_player_colors(_this);
    }
    // TODO: apply dyes to other players and their mimics
    else
    {
        // ...
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
static void update_color(from::CS::ChrIns *chr, const std::wstring &name,
                         const erdyes::color &color, const erdyes::intensity &intensity)
{
    auto new_modifier = from::CS::CSChrModelParamModifierModule::modifier{
        .name = name.data(),
        .value = {.material_id = 1,
                  .value1 = color.red,
                  .value2 = color.green,
                  .value3 = color.blue,
                  .value4 = 1.0f,
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
 * Updates all material parameters for the given character based on the main player's selected
 * colors
 */
static void update_main_player_colors(from::CS::ChrIns *chr)
{
    auto primary_color_index = erdyes::get_selected_option(erdyes::dye_target_type::primary_color);
    auto secondary_color_index =
        erdyes::get_selected_option(erdyes::dye_target_type::secondary_color);
    auto tertiary_color_index =
        erdyes::get_selected_option(erdyes::dye_target_type::tertiary_color);
    auto primary_intensity_index =
        erdyes::get_selected_option(erdyes::dye_target_type::primary_intensity);
    auto secondary_intensity_index =
        erdyes::get_selected_option(erdyes::dye_target_type::secondary_intensity);
    auto tertiary_intensity_index =
        erdyes::get_selected_option(erdyes::dye_target_type::tertiary_intensity);

    // Override the selected index with the focused dialog option, so you can hover over menu
    // options to preview them.
    auto talkscript_focused_entry = erdyes::get_talkscript_focused_entry();
    auto talkscript_dye_target = erdyes::get_talkscript_dye_target();
    switch (talkscript_dye_target)
    {
    case erdyes::dye_target_type::primary_color:
        // The minus 1 accounts for the "none" option in the color menu
        if (is_valid_color_index(talkscript_focused_entry - 1))
            primary_color_index = talkscript_focused_entry - 1;
        break;
    case erdyes::dye_target_type::secondary_color:
        if (is_valid_color_index(talkscript_focused_entry - 1))
            secondary_color_index = talkscript_focused_entry - 1;
        break;
    case erdyes::dye_target_type::tertiary_color:
        if (is_valid_color_index(talkscript_focused_entry - 1))
            tertiary_color_index = talkscript_focused_entry - 1;
        break;
    case erdyes::dye_target_type::primary_intensity:
        if (is_valid_intensity_index(talkscript_focused_entry))
            primary_intensity_index = talkscript_focused_entry;
        break;
    case erdyes::dye_target_type::secondary_intensity:
        if (is_valid_intensity_index(talkscript_focused_entry))
            secondary_intensity_index = talkscript_focused_entry;
        break;
    case erdyes::dye_target_type::tertiary_intensity:
        if (is_valid_intensity_index(talkscript_focused_entry))
            tertiary_intensity_index = talkscript_focused_entry;
        break;
    }

    // Update the player's material modifiers to have the RGB and intensity values of the chosen
    // colors
    if (is_valid_color_index(primary_color_index))
    {
        // Albedo 1 typically controls the largest portion of armor and weapon models
        update_color(chr, albedo1_material_ex_name, erdyes::colors[primary_color_index],
                     erdyes::intensities[primary_intensity_index]);
    }
    if (is_valid_color_index(secondary_color_index))
    {
        // Albedo 3 typically controls secondary materials and accents
        update_color(chr, albedo3_material_ex_name, erdyes::colors[secondary_color_index],
                     erdyes::intensities[secondary_intensity_index]);
    }
    if (is_valid_color_index(tertiary_color_index))
    {
        // Albedo 2 and 4 are both used less commonly for small details, so group them both in as
        // the "tertiary" color
        update_color(chr, albedo2_material_ex_name, erdyes::colors[tertiary_color_index],
                     erdyes::intensities[tertiary_intensity_index]);
        update_color(chr, albedo4_material_ex_name, erdyes::colors[tertiary_color_index],
                     erdyes::intensities[tertiary_intensity_index]);
    }
}

static void (*copy_player_character_data)(from::CS::PlayerIns *, from::CS::PlayerIns *);
static void copy_player_character_data_detour(from::CS::PlayerIns *target,
                                              from::CS::PlayerIns *source)
{
    copy_player_character_data(target, source);

    // When a player character is copied onto an NPC (Mimic Tear), store a number in the ChrAsm to
    // make sure dyes also apply to the mimic.
    auto world_chr_man = from::CS::WorldChrManImp::instance();
    if (world_chr_man && source == world_chr_man->main_player)
    {
        auto &chr_asm = target->player_game_data->equip_game_data.chr_asm;
        chr_asm.gear_param_ids.unused4 = player_copy_sentinel;
    }
}

void erdyes::apply_colors_init()
{
    modutils::hook(
        {
            .aob = "84 c0"                          // test al, al
                   "74 09"                          // je 11
                   "c6 87 ?? ?? ?? ?? 01"           // mov byte ptr [rdi + ????], 1
                   "eb 0a"                          // jmp 21
                   "c7 87 ?? ?? ?? ?? 00 00 00 00", // mov dword ptr [rdi + ????], 0
            .offset = -203,
        },
        cs_player_update_detour, cs_player_update);

    modutils::hook(
        {
            .aob = "c7 44 24 30 00 00 00 00" // mov [rsp + 0x30], 0x0
                   "48 8d 54 24 28"          // lea rdx, [rsp + 0x28]
                   "48 8b 8b 80 05 00 00"    // mov rcx, [rbx + 0x580]
                   "e8 ?? ?? ?? ??",         // call PlayerGameData::PopulatePcInfoBuffer
            .offset = -216,
        },
        copy_player_character_data_detour, copy_player_character_data);
}
