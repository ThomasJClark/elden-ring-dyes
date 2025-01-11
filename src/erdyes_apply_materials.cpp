/**
 * erdyes_apply_materials.hpp
 *
 * Applies colors to the player characters based on the current local and networked selections
 */
#include "erdyes_apply_materials.hpp"
#include "erdyes_local_player.hpp"
#include "erdyes_net_players.hpp"

#include <elden-x/chr/world_chr_man.hpp>
#include <elden-x/utils/modutils.hpp>
#include <spdlog/spdlog.h>
#include <string>

static const std::wstring albedo1_material_ex_name = L"[Albedo]_1_[Tint]";
static const std::wstring albedo2_material_ex_name = L"[Albedo]_2_[Tint]";
static const std::wstring albedo3_material_ex_name = L"[Albedo]_3_[Tint]";
static const std::wstring albedo4_material_ex_name = L"[Albedo]_4_[Tint]";

static constexpr int player_copy_sentinel = 0;

static float cumulative_time = 0.0f;
static constexpr float net_update_interval = 0.1f;

static void apply_colors(er::CS::ChrIns *, const erdyes::state::dye_values &);

// CS::PlayerIns::Update(float delta_time)
static void (*cs_player_update)(er::CS::PlayerIns *, float);
static void cs_player_update_detour(er::CS::PlayerIns *_this, float delta_time)
{
    cs_player_update(_this, delta_time);

    auto &chr_asm = _this->game_data->equip_game_data.chr_asm;

    if (_this == er::CS::WorldChrManImp::instance()->main_player)
    {
        // Check the loaded save slot for the latest dye selections
        erdyes::local_player::update();

        // Apply the dye selections locally to the main player
        auto local_player_dyes = erdyes::local_player::get_selected_dyes();
        apply_colors(_this, local_player_dyes);

        // Also periodically send the dye selections to other connected players, so their games can
        // show the dyes if they have the mod installed
        cumulative_time += delta_time;
        if (cumulative_time > net_update_interval)
        {
            cumulative_time -= net_update_interval;
            erdyes::net_players::send_messages(local_player_dyes);
        }
    }
    // Apply the main player logic to any mimic tears as well
    else if (chr_asm.gear_param_ids.unused4 == player_copy_sentinel)
    {
        auto local_player_dyes = erdyes::local_player::get_selected_dyes();
        apply_colors(_this, local_player_dyes);
    }
    else
    {
        auto network_session = _this->session_holder.network_session;
        if (network_session)
        {
            // Check the network buffer for dye updates from other players
            erdyes::net_players::receive_messages();

            // Apply the dye selections we've received from this player
            auto net_player_dyes =
                erdyes::net_players::get_selected_dyes(network_session->steam_id);
            apply_colors(_this, net_player_dyes);
        }
    }
}

/**
 * Updates all material parameters for the given character based on the given selected colors
 */
static void apply_colors(er::CS::ChrIns *chr, const erdyes::state::dye_values &values)
{
    auto apply_color = [&](const std::wstring &name, const erdyes::state::dye_value &value) {
        auto new_modifier = er::CS::CSChrModelParamModifierModule::modifier{
            .name = name.data(),
            .value = {.material_id = 1,
                      .value1 = value.red,
                      .value2 = value.green,
                      .value3 = value.blue,
                      .value4 = 1.0f,
                      .value5 = value.intensity},
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

    if (values.primary.is_applied)
    {
        // Albedo 1 typically controls the largest portion of armor and weapon models
        apply_color(albedo1_material_ex_name, values.primary);
    }
    if (values.secondary.is_applied)
    {
        // Albedo 3 typically controls secondary materials and accents
        apply_color(albedo3_material_ex_name, values.secondary);
    }
    if (values.tertiary.is_applied)
    {
        // Albedo 2 and 4 are both used less commonly for small details, so group them both in as
        // the "tertiary" color
        apply_color(albedo2_material_ex_name, values.tertiary);
        apply_color(albedo4_material_ex_name, values.tertiary);
    }
}

static void (*copy_player_character_data)(er::CS::PlayerIns *, er::CS::PlayerIns *);
static void copy_player_character_data_detour(er::CS::PlayerIns *target, er::CS::PlayerIns *source)
{
    copy_player_character_data(target, source);

    // When a player character is copied onto an NPC (Mimic Tear), store a number in the ChrAsm to
    // make sure dyes also apply to the mimic.
    auto world_chr_man = er::CS::WorldChrManImp::instance();
    if (world_chr_man && source == world_chr_man->main_player)
    {
        auto &chr_asm = target->game_data->equip_game_data.chr_asm;
        chr_asm.gear_param_ids.unused4 = player_copy_sentinel;
    }
}

void erdyes::apply_materials_init()
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
