#include "erdyes_params.hpp"

#include <elden-x/paramdef/MATERIAL_EX_PARAM_ST.hpp>
#include <elden-x/paramdef/SP_EFFECT_PARAM_ST.hpp>
#include <elden-x/paramdef/SP_EFFECT_VFX_PARAM_ST.hpp>
#include <elden-x/utils/modutils.hpp>

/**
 * @return a SpEffectParam struct with fields set to reasonable defaults
 */
static constexpr from::paramdef::SP_EFFECT_PARAM_ST make_sp_effect(int sp_effect_vfx_id)
{
    return from::paramdef::SP_EFFECT_PARAM_ST{
        .effectEndurance = -1.0f,
        .fallDamageRate = 1.0f,
        .soulRate = 1.0f,
        .equipWeightChangeRate = 1.0f,
        .allItemWeightChangeRate = 1.0f,
        .soulStealRate = 1.0f,
        .lifeReductionRate = 1.0f,
        .hpRecoverRate = 1.0f,
        .effectTargetSelf = true,
        .effectTargetFriend = true,
        .effectTargetPlayer = true,
        .effectTargetAI = true,
        .effectTargetLive = true,
        .effectTargetGhost = true,
        .vowType0 = true,
        .vowType1 = true,
        .vowType2 = true,
        .vowType3 = true,
        .vowType4 = true,
        .vowType5 = true,
        .vowType6 = true,
        .vowType7 = true,
        .vowType8 = true,
        .vowType9 = true,
        .vowType10 = true,
        .vowType11 = true,
        .vowType12 = true,
        .vowType13 = true,
        .vowType14 = true,
        .vowType15 = true,
        .effectTargetSelfTarget = true,
        .vfxId = sp_effect_vfx_id,
    };
}

static auto primary_color_sp_effect = make_sp_effect(erdyes::primary_color_sp_effect_vfx_id);
static auto secondary_color_sp_effect = make_sp_effect(erdyes::secondary_color_sp_effect_vfx_id);
static auto tertiary_color_sp_effect = make_sp_effect(erdyes::tertiary_color_sp_effect_vfx_id);

static auto primary_color_sp_effect_vfx = from::paramdef::SP_EFFECT_VFX_PARAM_ST{
    .playCategory = 7, .materialParamId = erdyes::primary_color_material_ex_id};
static auto secondary_color_sp_effect_vfx = from::paramdef::SP_EFFECT_VFX_PARAM_ST{
    .playCategory = 7, .materialParamId = erdyes::secondary_color_material_ex_id};
static auto tertiary_color_sp_effect_vfx = from::paramdef::SP_EFFECT_VFX_PARAM_ST{
    .playCategory = 7, .materialParamId = erdyes::tertiary_color_material_ex_id};

static auto primary_color_material_ex = from::paramdef::MATERIAL_EX_PARAM_ST{};
static auto secondary_color_material_ex = from::paramdef::MATERIAL_EX_PARAM_ST{};
static auto tertiary_color_material_ex = from::paramdef::MATERIAL_EX_PARAM_ST{};

struct special_effect_param_lookup
{
    from::paramdef::SP_EFFECT_PARAM_ST *row;
    int id;
    unsigned char unkc{4};
};

struct special_effect_vfx_param_lookup
{
    int id;
    from::paramdef::SP_EFFECT_VFX_PARAM_ST *row;
    unsigned short unk10{1};
};

// CS::SoloParamRepositoryImp::FindSpEffectParam()
static void (*find_special_effect_param)(special_effect_param_lookup &, int);
static void find_special_effect_param_detour(special_effect_param_lookup &result, int id)
{
    find_special_effect_param(result, id);
    if (!result.row)
    {
        if (id == erdyes::primary_color_sp_effect_id)
            result = {&primary_color_sp_effect, id};
        else if (id == erdyes::secondary_color_sp_effect_id)
            result = {&secondary_color_sp_effect, id};
        else if (id == erdyes::tertiary_color_sp_effect_id)
            result = {&tertiary_color_sp_effect, id};
    }
}

// CS::SoloParamRepositoryImp::FindSpEffectVfxParam()
void (*find_special_effect_vfx_param)(special_effect_vfx_param_lookup &, int);
void find_special_effect_vfx_param_detour(special_effect_vfx_param_lookup &result, int id)
{
    find_special_effect_vfx_param(result, id);
    if (!result.row)
    {
        if (id == erdyes::primary_color_sp_effect_vfx_id)
            result = {id, &primary_color_sp_effect_vfx};
        else if (id == erdyes::secondary_color_sp_effect_vfx_id)
            result = {id, &secondary_color_sp_effect_vfx};
        else if (id == erdyes::tertiary_color_sp_effect_vfx_id)
            result = {id, &tertiary_color_sp_effect_vfx};
    }
}

void erdyes::setup_params()
{
    auto initialize_param_name = [](from::paramdef::MATERIAL_EX_PARAM_ST &param,
                                    std::wstring const &name) {
        std::copy(name.begin(), name.end(), param.paramName);
    };

    initialize_param_name(primary_color_material_ex, erdyes::primary_color_material_ex_name);
    initialize_param_name(secondary_color_material_ex, erdyes::secondary_color_material_ex_name);
    initialize_param_name(tertiary_color_material_ex, erdyes::tertiary_color_material_ex_name);

    modutils::hook(
        {
            .aob = "41 8d 50 0f"        // lea edx, [r8 + 15]
                   "e8 ?? ?? ?? ??"     // call SoloParamRepositoryImp::GetParamResCap
                   "48 85 c0"           // test rax, rax
                   "0f 84 ?? ?? ?? ??", // jz end_lbl
            .offset = -114,
        },
        find_special_effect_param_detour, find_special_effect_param);

    modutils::hook(
        {
            .aob = "41 8d 50 10"    // lea edx, [r8 + 16]
                   "e8 ?? ?? ?? ??" // call SoloParamRepositoryImp::GetParamResCap
                   "48 85 c0"       // test rax, rax
                   "74 ??",         // jz end_lbl
            .offset = -106,
        },
        find_special_effect_vfx_param_detour, find_special_effect_vfx_param);
}
