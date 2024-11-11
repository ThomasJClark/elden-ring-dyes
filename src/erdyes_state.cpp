/**
 * erdyes_player_state.hpp
 *
 * Keeps track of the current dye selections for the player, saving and loading them and exposing
 * the results to the talkscript, messages, and color application systems.
 */
#include "erdyes_state.hpp"

#include <elden-x/chr/player_game_data.hpp>
#include <elden-x/paramdef/EQUIP_PARAM_GOODS_ST.hpp>
#include <elden-x/utils/modutils.hpp>
#include <spdlog/spdlog.h>

static constexpr unsigned int item_type_goods = 0x40000000;
static constexpr unsigned char goods_type_hidden = 13;

static constexpr int default_color_index = -1;
static constexpr int default_intensity_index = 3;

// A hidden good that's saved to the player's inventory in order to persist dye settings
static auto dummy_good =
    from::paramdef::EQUIP_PARAM_GOODS_ST{.maxNum = 1, .goodsType = goods_type_hidden};

static constexpr int dummy_good_primary_color_start = 6700000;
static constexpr int dummy_good_secondary_color_start = 6710000;
static constexpr int dummy_good_tertiary_color_start = 6720000;
static constexpr int dummy_good_primary_intensity_start = 6730000;
static constexpr int dummy_good_secondary_intensity_start = 6740000;
static constexpr int dummy_good_tertiary_intensity_start = 6750000;

std::vector<erdyes::color> erdyes::colors;
std::vector<erdyes::intensity> erdyes::intensities;

// TEMPORARY in-memory selections
int erdyes::primary_color_index = default_color_index;
int erdyes::secondary_color_index = default_color_index;
int erdyes::tertiary_color_index = default_color_index;
int erdyes::primary_intensity_index = default_intensity_index;
int erdyes::secondary_intensity_index = default_intensity_index;
int erdyes::tertiary_intensity_index = default_intensity_index;

// AddRemoveItem(ItemType itemType, unsigned int itemId, int quantity)
typedef void add_remove_item_fn(unsigned long long item_type, unsigned int item_id, int quantity);
static add_remove_item_fn *add_remove_item;

// CS::EquipInventoryData::GetInventoryId(int itemId)
typedef int get_inventory_id_fn(from::CS::EquipInventoryData *, int *item_id);
static get_inventory_id_fn *get_inventory_id;

struct get_equip_param_goods_result
{
    int id;
    int unk;
    from::paramdef::EQUIP_PARAM_GOODS_ST *row;
};

static void (*get_equip_param_goods)(get_equip_param_goods_result *result, int id);

// Hook for CS::SoloParamRepositoryImp::GetEquipParamGoods()
static void get_equip_param_goods_detour(get_equip_param_goods_result *result, int id)
{
    if ((id >= dummy_good_primary_color_start &&
         id < dummy_good_primary_color_start + erdyes::colors.size()) ||
        (id >= dummy_good_secondary_color_start &&
         id < dummy_good_secondary_color_start + erdyes::colors.size()) ||
        (id >= dummy_good_tertiary_color_start &&
         id < dummy_good_tertiary_color_start + erdyes::colors.size()) ||
        (id >= dummy_good_primary_intensity_start &&
         id < dummy_good_primary_intensity_start + erdyes::intensities.size()) ||
        (id >= dummy_good_secondary_intensity_start &&
         id < dummy_good_secondary_intensity_start + erdyes::intensities.size()) ||
        (id >= dummy_good_tertiary_intensity_start &&
         id < dummy_good_tertiary_intensity_start + erdyes::intensities.size()))
    {
        spdlog::info("yeah dummy good {}", id);
        result->id = id;
        result->row = &dummy_good;
        result->unk = 3;
        return;
    }

    get_equip_param_goods(result, id);
}

void erdyes::state_init()
{
    add_remove_item = modutils::scan<add_remove_item_fn>({
        .aob = "8b 99 90 01 00 00" // mov ebx, [rcx + 0x190]
               "41 83 c8 ff"       // or r8d, -1
               "8b d3"             // mov edx, ebx
               "b9 00 00 00 40"    // mov ecx, item_type_goods
               "e8 ?? ?? ?? ??",   // call AddRemoveItem
        .offset = 17,
        .relative_offsets = {{1, 5}},
    });

    get_inventory_id = modutils::scan<get_inventory_id_fn>({
        .aob = "48 8d 8f 58 01 00 00" // lea rcx, [rdi + 0x158]
               "e8 ?? ?? ?? ??"       // call CS::EquipInventoryData::GetInventoryId
               "8b d8"                // mov ebx, eax
               "85 c0"                // test eax, eax
               "78 6a",               // js label
        .offset = 7,
        .relative_offsets = {{1, 5}},
    });

    // Hook GetEquipParamGoods() to return fake items used to store the dye state in the player's
    // inventory
    modutils::hook(
        {
            .aob = "41 8d 50 03"        // lea edx, [r8 + 3]
                   "e8 ?? ?? ?? ??"     // call SoloParamRepositoryImp::GetParamResCap
                   "48 85 c0"           // test rax rax
                   "0f 84 ?? ?? ?? ??", // jz end_lbl
            .offset = -106,
        },
        get_equip_param_goods_detour, get_equip_param_goods);

    // Add 10 hardcoded intensity options. The color options are added by erdyes_config.cpp based
    // on the .ini file
    erdyes::add_intensity_option(L"1", L"#1e1e1e", 0.125f);
    erdyes::add_intensity_option(L"2", L"#3d3d3d", 0.25f);
    erdyes::add_intensity_option(L"3", L"#4d4d4d", 0.5f);
    erdyes::add_intensity_option(L"4", L"#656565", 1.0f);
    erdyes::add_intensity_option(L"5", L"#7f7f7f", 2.0f);
    erdyes::add_intensity_option(L"6", L"#9a9a9a", 4.0f);
    erdyes::add_intensity_option(L"7", L"#b2b2b2", 8.0f);
    erdyes::add_intensity_option(L"8", L"#c9c9c9", 16.0f);
    erdyes::add_intensity_option(L"9", L"#e1e1e1", 32.0f);
    erdyes::add_intensity_option(L"10", L"#ffffff", 64.0f);
}

/**
 * Format one of the color or intensity menu options. This contains an image (either blank for
 * padding or a dot to show it's selected), a colored block character, and the option name.
 */
static constexpr std::wstring format_message(std::wstring const &hex_code, std::wstring const &name,
                                             bool selected)
{
    auto img_src = selected ? L"MENU_Lockon_01a.png" : L"MENU_DummyTransparent.dds";
    return std::wstring{L"<IMG SRC='img://"} + img_src +
           L"' WIDTH='20' HEIGHT='20' HSPACE='0' VSPACE='-1'>" +
           L"<FONT FACE='Bingus Sans' COLOR='" + hex_code + L"'>" + L"*</FONT> " + name;
};

void erdyes::add_color_option(const std::wstring &name, const std::wstring &hex_code, float r,
                              float g, float b)
{
    colors.emplace_back(format_message(hex_code, name, true), format_message(hex_code, name, false),
                        r, g, b);
}

void erdyes::add_intensity_option(const std::wstring &name, const std::wstring &hex_code, float i)
{
    intensities.emplace_back(format_message(hex_code, name, true),
                             format_message(hex_code, name, false), i);
}

void erdyes::set_selected_option(erdyes::dye_target_type dye_target, int index)
{
    auto [base_goods_id, count] = [dye_target]() -> std::pair<int, size_t> {
        switch (dye_target)
        {
        case erdyes::dye_target_type::primary_color:
            return {dummy_good_primary_color_start, erdyes::colors.size()};
        case erdyes::dye_target_type::secondary_color:
            return {dummy_good_secondary_color_start, erdyes::colors.size()};
        case erdyes::dye_target_type::tertiary_color:
            return {dummy_good_tertiary_color_start, erdyes::colors.size()};
        case erdyes::dye_target_type::primary_intensity:
            return {dummy_good_primary_intensity_start, erdyes::intensities.size()};
        case erdyes::dye_target_type::secondary_intensity:
            return {dummy_good_secondary_intensity_start, erdyes::intensities.size()};
        case erdyes::dye_target_type::tertiary_intensity:
            return {dummy_good_tertiary_intensity_start, erdyes::intensities.size()};
        }
        return {-1, 0};
    }();

    if (base_goods_id == -1)
        return;

    // Remove any existing dummy items in the given range to clear out existing dye selections
    for (int i = 0; i < count; i++)
    {
        int goods_id = base_goods_id + i;
        int item_id = item_type_goods + goods_id;
        // if (get_inventory_id(nullptr, &item_id) != -1)
        {
            spdlog::info("Remove {}", goods_id);
            add_remove_item(item_type_goods, goods_id, -1); // not removing???
        }
    }

    // If a new selection was chosen, add an item to the player's inventory to store this selection
    if (index != -1)
    {
        spdlog::info("Add {}", base_goods_id + index);
        add_remove_item(item_type_goods, base_goods_id + index, 1);
    }
    // If "none" was chosen for a color, also remove the item for the corresponding intensity
    else if (dye_target == erdyes::dye_target_type::primary_color)
        set_selected_option(erdyes::dye_target_type::primary_intensity, -1);
    else if (dye_target == erdyes::dye_target_type::secondary_color)
        set_selected_option(erdyes::dye_target_type::secondary_intensity, -1);
    else if (dye_target == erdyes::dye_target_type::tertiary_color)
        set_selected_option(erdyes::dye_target_type::tertiary_intensity, -1);
}