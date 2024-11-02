#pragma once

#include <string>

namespace erdyes
{

constexpr int primary_color_speffect_id = 67000000;
constexpr int secondary_color_speffect_id = 67000001;
constexpr int tertiary_color_speffect_id = 67000002;

const std::wstring primary_color_material_ex_name = L"[Albedo]_1_[Tint]";
const std::wstring secondary_color_material_ex_name = L"[Albedo]_3_[Tint]";
const std::wstring tertiary_color_material_ex_name = L"[Albedo]_4_[Tint]";

void apply_colors_init();
}
