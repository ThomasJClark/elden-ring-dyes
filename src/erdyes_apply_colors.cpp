#include "erdyes_apply_colors.hpp"
#include "erdyes_state.hpp"
#include "erdyes_talkscript.hpp"

#include <elden-x/chr/world_chr_man.hpp>
#include <elden-x/utils/modutils.hpp>
#include <spdlog/spdlog.h>
#include <string>
#include <thread>

static void update_colors(from::CS::ChrIns *);

static from::CS::WorldChrManImp **world_chr_man_addr;

// int CS::ChrIns::ApplyEffect(unsigned int speffect_id, bool unk)
typedef int apply_speffect_fn(from::CS::ChrIns *, unsigned int speffect_id, bool);
static apply_speffect_fn *apply_speffect;

// int CS::ChrIns::ClearEffect(unsigned int speffect_id)
typedef int clear_speffect_fn(from::CS::ChrIns *, unsigned int speffect_id);
static clear_speffect_fn *clear_speffect;

// bool CS::SpecialEffect::HasSpecialEffectId(unsigned int speffect_id)
typedef bool has_speffect_fn(from::CS::SpecialEffect *, unsigned int speffect_id);
static has_speffect_fn *has_speffect;

// CS::ChrIns::Update(float delta_time)
static void (*chr_ins_update)(from::CS::ChrIns *, float);
static void chr_ins_update_detour(from::CS::ChrIns *_this, float delta_time)
{
    auto chr = *world_chr_man_addr ? (*world_chr_man_addr)->main_player : nullptr;
    if (chr && chr->modules && chr->modules->model_param_modifier_module)
    {
        update_colors(chr);
    }

    chr_ins_update(_this, delta_time);
}

static bool is_valid_color_index(int index)
{
    return index >= 0 && index < erdyes::state::colors.size();
}

/**
 * Applies SpEffects and updates material modifiers for the given character based on the selected
 * colors
 */
static void update_colors(from::CS::ChrIns *chr)
{
    auto primary_color_index = erdyes::state::primary_color_index;
    auto secondary_color_index = erdyes::state::secondary_color_index;
    auto tertiary_color_index = erdyes::state::tertiary_color_index;

    // Override the selected index with the focused dialog option, so you can hover over colors
    // to preview them
    auto focused_entry = erdyes::talkscript::get_focused_entry();
    if (is_valid_color_index(focused_entry))
    {
        switch (erdyes::talkscript::dye_target)
        {
        case erdyes::talkscript::dye_target_type::primary_color:
            primary_color_index = focused_entry;
            break;
        case erdyes::talkscript::dye_target_type::secondary_color:
            secondary_color_index = focused_entry;
            break;
        case erdyes::talkscript::dye_target_type::tertiary_color:
            tertiary_color_index = focused_entry;
            break;
        }
    }

    auto set_speffect = [chr](unsigned int speffect_id, bool should_have_effect) {
        if (has_speffect(chr->special_effects, speffect_id) != should_have_effect)
        {
            spdlog::info("speffect {} -> {}", speffect_id, should_have_effect ? "on" : "off");
            if (should_have_effect)
                apply_speffect(chr, speffect_id, false);
            else
                clear_speffect(chr, speffect_id);
        }
    };

    auto set_color = [chr](const std::wstring &name, int color_index) {
        if (is_valid_color_index(color_index))
        {
            for (auto &modifier : chr->modules->model_param_modifier_module->modifiers)
            {
                if (modifier.name == name)
                {
                    auto &color = erdyes::state::colors[color_index];
                    modifier.value.material_id = 1;
                    modifier.value.value1 = color.red;
                    modifier.value.value2 = color.green;
                    modifier.value.value3 = color.blue;
                    modifier.value.value4 = 1.0f;
                    modifier.value.value5 = 1.0f;
                    return;
                }
            }
        }
    };

    // Ensure the player has or doesn't have the correct SpEffects for any chosen colors
    set_speffect(erdyes::primary_color_speffect_id, is_valid_color_index(primary_color_index));
    set_speffect(erdyes::secondary_color_speffect_id, is_valid_color_index(secondary_color_index));
    set_speffect(erdyes::tertiary_color_speffect_id, is_valid_color_index(tertiary_color_index));

    // Update the player's material modifiers to have the RGB and intensity values of the chosen
    // colors
    set_color(erdyes::primary_color_material_ex_name, primary_color_index);
    set_color(erdyes::secondary_color_material_ex_name, secondary_color_index);
    set_color(erdyes::tertiary_color_material_ex_name, tertiary_color_index);
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

    auto disable_enable_grace_warp_address = modutils::scan<unsigned char>({
        .aob = "45 33 c0"        // xor r8d, r8d
               "ba ae 10 00 00"  // mov edx, 4270 ; Disable Grace Warp
               "48 8b cf"        // mov rcx, rdi
               "e8 ?? ?? ?? ??"  // call ChrIns::ApplyEffect
               "eb 16"           // jmp end_label
               "e8 ?? ?? ?? ??"  // call CS::SpecialEffect::HasSpecialEffectIdHasSpEffect
               "84 c0"           // test al, al
               "74 0d"           // jz end_label
               "ba ae 10 00 00"  // mov edx, 4270 ; Disable Grace Warp
               "48 8b cf"        // mov rcx, rdi
               "e8 ?? ?? ?? ??", // call ChrIns::ClearSpEffect
    });

    apply_speffect = modutils::scan<apply_speffect_fn>({
        .address = disable_enable_grace_warp_address + 11,
        .relative_offsets = {{1, 5}},
    });

    has_speffect = modutils::scan<has_speffect_fn>({
        .address = disable_enable_grace_warp_address + 18,
        .relative_offsets = {{1, 5}},
    });

    clear_speffect = modutils::scan<clear_speffect_fn>({
        .address = disable_enable_grace_warp_address + 35,
        .relative_offsets = {{1, 5}},
    });

    // TODO aob
    modutils::hook({.offset = 0x65a060}, chr_ins_update_detour, chr_ins_update);
}
