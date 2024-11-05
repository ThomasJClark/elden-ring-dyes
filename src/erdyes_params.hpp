#pragma once

#include <string>

namespace erdyes
{

constexpr int primary_color_sp_effect_id = 67000000;
constexpr int secondary_color_sp_effect_id = 67000001;
constexpr int tertiary_color_sp_effect_id = 67000002;

constexpr int primary_color_sp_effect_vfx_id = 67000000;
constexpr int secondary_color_sp_effect_vfx_id = 67000001;
constexpr int tertiary_color_sp_effect_vfx_id = 67000002;

constexpr int primary_color_material_ex_id = 6700000;
constexpr int secondary_color_material_ex_id = 6700001;
constexpr int tertiary_color_material_ex_id = 6700002;

const std::wstring primary_color_material_ex_name = L"[Albedo]_1_[Tint]";
const std::wstring secondary_color_material_ex_name = L"[Albedo]_3_[Tint]";
const std::wstring tertiary_color_material_ex_name = L"[Albedo]_4_[Tint]";

/**
 * Patch several SoloParamRepositoryImp() lookup methods to insert params used by the mod
 */
void setup_params();

}
