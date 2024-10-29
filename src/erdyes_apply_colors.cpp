#include "erdyes_apply_colors.hpp"
#include "erdyes_state.hpp"
#include "erdyes_talkscript.hpp"
#include "from/chr.hpp"
#include "from/world_chr_man.hpp"

#include <elden-x/utils/modutils.hpp>
#include <spdlog/spdlog.h>
#include <string>
#include <thread>

static from::CS::WorldChrManImp **world_chr_man_addr;

static void apply_colors()
{
    auto chr = *world_chr_man_addr ? (*world_chr_man_addr)->main_player : nullptr;
    if (!chr || !chr->modules || !chr->modules->model_param_modifier_module)
    {
        return;
    }

    auto &modifiers = chr->modules->model_param_modifier_module->modifiers;

    auto primary_color_index = erdyes::state::primary_color_index;
    auto secondary_color_index = erdyes::state::secondary_color_index;
    auto tertiary_color_index = erdyes::state::tertiary_color_index;

    // Override the selected index with the focused dialog option, so you can hover over colors
    // to preview them
    auto focused_entry = erdyes::talkscript::get_focused_entry();
    if (focused_entry != -1)
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

    for (auto &modifier : modifiers)
    {
        auto name = std::wstring_view{modifier.name};
        if (name == L"[Albedo]_1_[Tint]")
        {
            if (primary_color_index != -1)
            {
                auto &color = erdyes::state::colors[primary_color_index];
                modifier.value = {
                    .material_id = 1,
                    .value1 = color.red,
                    .value2 = color.green,
                    .value3 = color.blue,
                    .value4 = 1.0f,
                    .value5 = 1.0f,
                };
            }
        }
        else if (name == L"[Albedo]_2_[Tint]")
        {
            if (secondary_color_index != -1)
            {
                auto &color = erdyes::state::colors[secondary_color_index];
                modifier.value = {
                    .material_id = 1,
                    .value1 = color.red,
                    .value2 = color.green,
                    .value3 = color.blue,
                    .value4 = 1.0f,
                    .value5 = 1.0f,
                };
            }
        }
        else if (name == L"[Albedo]_3_[Tint]")
        {
            if (tertiary_color_index != -1)
            {
                auto &color = erdyes::state::colors[tertiary_color_index];
                modifier.value = {
                    .material_id = 1,
                    .value1 = color.red,
                    .value2 = color.green,
                    .value3 = color.blue,
                    .value4 = 1.0f,
                    .value5 = 1.0f,
                };
            }
        }
    }
}

// CS::ChrIns::update(float delta_time)
static void (*chr_ins_update)(from::CS::ChrIns *, float);
static void chr_ins_update_detour(from::CS::ChrIns *_this, float delta_time)
{
    apply_colors();
    chr_ins_update(_this, delta_time);
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

    // TODO aob
    modutils::hook({.offset = 0x65a060}, chr_ins_update_detour, chr_ins_update);
}
