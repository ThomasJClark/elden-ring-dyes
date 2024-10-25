#include "erdyes_apply_colors.hpp"
#include "erdyes_config.hpp"
#include "from/chr.hpp"
#include "from/taskgroup.hpp"
#include "from/world_chr_man.hpp"

#include <array>
#include <chrono>
#include <cmath>
#include <elden-x/utils/modutils.hpp>
#include <span>
#include <spdlog/spdlog.h>
#include <string>
#include <thread>
#include <unordered_set>

static from::CS::WorldChrManImp **world_chr_man_addr;

float t = 0.0f;

// CS::ChrIns::update(float delta_time)
static void (*chr_ins_update)(from::CS::ChrIns *, float);
static void chr_ins_update_detour(from::CS::ChrIns *_this, float delta_time)
{
    auto chr = *world_chr_man_addr ? ((*world_chr_man_addr)->main_player) : nullptr;
    auto &modifiers = chr->modules->model_param_modifier_module->modifiers;

    t += delta_time;

    auto color = erdyes::config::colors[(int)std::roundf(t) % erdyes::config::colors.size()];

    for (auto &modifier : modifiers)
    {
        auto name = std::wstring_view{modifier.name};
        if (name == L"[Albedo]_1_[Tint]")
        {
            modifier.value = {
                .material_id = 1,
                .value1 = color.red,
                .value2 = color.green,
                .value3 = color.blue,
                .value4 = 1.0f,
                .value5 = 1.0f,
            };
        }
        else if (name == L"[Albedo]_2_[Tint]")
        {
            modifier.value = {
                .material_id = 1,
                .value1 = 0.0f,
                .value2 = 1.0f,
                .value3 = 0.0f,
                .value4 = 1.0f,
                .value5 = 1.0f,
            };
        }
        else if (name == L"[Albedo]_3_[Tint]")
        {
            modifier.value = {
                .material_id = 1,
                .value1 = 0.0f,
                .value2 = 0.0f,
                .value3 = 1.0f,
                .value4 = 1.0f,
                .value5 = 1.0f,
            };
        }
        else if (name == L"[Albedo]_4_[Tint]")
        {
            modifier.value = {
                .material_id = 1,
                .value1 = 1.0f,
                .value2 = 1.0f,
                .value3 = 0.0f,
                .value4 = 1.0f,
                .value5 = 1.0f,
            };
        }
    }

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

    modutils::hook({.offset = 0x65a060}, chr_ins_update_detour, chr_ins_update);
}

void erdyes::apply_colors_loop()
{
}
