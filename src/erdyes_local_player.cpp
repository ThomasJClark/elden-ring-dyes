/**
 * erdyes_local_player.cpp
 *
 * Keeps track of the current dye selections for the player, saving and loading them and exposing
 * the results to the talkscript, messages, and color application systems.
 */
#include "erdyes_local_player.hpp"
#include "erdyes_messages.hpp"
#include "erdyes_talkscript.hpp"

#include <elden-x/chr/world_chr_man.hpp>
#include <elden-x/paramdef/EQUIP_PARAM_GOODS_ST.hpp>
#include <elden-x/utils/modutils.hpp>
#include <spdlog/spdlog.h>

static constexpr unsigned int item_type_goods = 0x40000000;
static constexpr unsigned char goods_type_hidden = 13;

static constexpr int default_color_index = -1;
static constexpr int default_intensity_index = 3;

// A hidden good that's saved to the player's inventory in order to persist dye settings
static auto dummy_good = er::paramdef::equip_param_goods_st{
    .maxNum = 1,
    .goodsType = goods_type_hidden,
};
static constexpr int dummy_good_primary_color_start = 6700000;
static constexpr int dummy_good_secondary_color_start = 6710000;
static constexpr int dummy_good_tertiary_color_start = 6720000;
static constexpr int dummy_good_primary_intensity_start = 6730000;
static constexpr int dummy_good_secondary_intensity_start = 6740000;
static constexpr int dummy_good_tertiary_intensity_start = 6750000;

std::array<std::wstring, 6> erdyes::dye_target_messages;

std::vector<erdyes::color> erdyes::colors;
std::vector<erdyes::intensity> erdyes::intensities;

static erdyes::state::dye_values local_player_dyes;

static bool is_valid_color_index(int index)
{
    return index >= 0 && index < erdyes::colors.size();
}

static bool is_valid_intensity_index(int index)
{
    return index >= 0 && index < erdyes::intensities.size();
}

static bool is_color(erdyes::dye_target_type dye_target)
{
    return dye_target == erdyes::dye_target_type::primary_color ||
           dye_target == erdyes::dye_target_type::secondary_color ||
           dye_target == erdyes::dye_target_type::tertiary_color;
}

// AddRemoveItem(ItemType itemType, unsigned int itemId, int quantity)
typedef void add_remove_item_fn(unsigned long long item_type, unsigned int item_id, int quantity);
static add_remove_item_fn *add_remove_item;

// CS::EquipInventoryData::GetInventoryId(int itemId)
typedef int get_inventory_id_fn(er::CS::EquipInventoryData *, int *item_id);
static get_inventory_id_fn *get_inventory_id;

struct get_equip_param_goods_result
{
    int id;
    int unk;
    er::paramdef::equip_param_goods_st *row;
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
        result->id = id;
        result->row = &dummy_good;
        result->unk = 3;
        return;
    }

    get_equip_param_goods(result, id);
}

void erdyes::local_player::init()
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
    erdyes::local_player::add_intensity_option(L"1", L"#1e1e1e", 0.125f);
    erdyes::local_player::add_intensity_option(L"2", L"#3d3d3d", 0.25f);
    erdyes::local_player::add_intensity_option(L"3", L"#4d4d4d", 0.5f);
    erdyes::local_player::add_intensity_option(L"4", L"#656565", 1.0f);
    erdyes::local_player::add_intensity_option(L"5", L"#7f7f7f", 2.0f);
    erdyes::local_player::add_intensity_option(L"6", L"#9a9a9a", 4.0f);
    erdyes::local_player::add_intensity_option(L"7", L"#b2b2b2", 8.0f);
    erdyes::local_player::add_intensity_option(L"8", L"#c9c9c9", 16.0f);
    erdyes::local_player::add_intensity_option(L"9", L"#e1e1e1", 32.0f);
    erdyes::local_player::add_intensity_option(L"10", L"#ffffff", 64.0f);
}

void erdyes::local_player::update()
{
    auto update_dye_value = [](erdyes::state::dye_value &dye_value,
                               erdyes::dye_target_type color_target,
                               erdyes::dye_target_type intensity_target) {
        auto color_index = get_selected_index(color_target);
        if (is_valid_color_index(color_index))
        {
            auto intensity_index = get_selected_index(intensity_target);
            dye_value.is_applied = true;
            dye_value.red = erdyes::colors[color_index].red;
            dye_value.green = erdyes::colors[color_index].green;
            dye_value.blue = erdyes::colors[color_index].blue;
            dye_value.intensity = erdyes::intensities[intensity_index].intensity;
        }
        else
        {
            dye_value.is_applied = false;
        }
    };

    update_dye_value(local_player_dyes.primary, erdyes::dye_target_type::primary_color,
                     erdyes::dye_target_type::primary_intensity);
    update_dye_value(local_player_dyes.secondary, erdyes::dye_target_type::secondary_color,
                     erdyes::dye_target_type::secondary_intensity);
    update_dye_value(local_player_dyes.tertiary, erdyes::dye_target_type::tertiary_color,
                     erdyes::dye_target_type::tertiary_intensity);
}

/**
 * Render a string of text that displays as a colored rectangle.
 */
static constexpr std::wstring format_color_block(const std::wstring &hex_code)
{
    // This font face doesn't exist - Scaleform has no fallback font and will render a convenient
    // rectangle character
    return L"<FONT FACE='Bingus Sans' COLOR='" + hex_code + L"'>" + L"*</FONT> ";
}

/**
 * @returns the range of dummy goods IDs used to store the given dye selection
 */
static std::pair<int, size_t> get_dye_target_goods_range(erdyes::dye_target_type dye_target)
{
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
};

void erdyes::local_player::add_color_option(const std::wstring &name, const std::wstring &hex_code,
                                            float r, float g, float b)
{
    auto color_block = format_color_block(hex_code);
    auto label = color_block + name;
    colors.emplace_back(color_block, erdyes::format_option_message(label, true),
                        erdyes::format_option_message(label, false), r, g, b);
}

void erdyes::local_player::add_intensity_option(const std::wstring &name,
                                                const std::wstring &hex_code, float i)
{
    auto color_block = format_color_block(hex_code);
    auto label = color_block + name;
    intensities.emplace_back(color_block, erdyes::format_option_message(label, true),
                             erdyes::format_option_message(label, false), i);
}

const erdyes::state::dye_values &erdyes::local_player::get_selected_dyes()
{
    return local_player_dyes;
}

void erdyes::local_player::update_dye_target_messages()
{
    auto set_messages = [](dye_target_type color_target, dye_target_type intensity_target,
                           const std::wstring &color_msg, const std::wstring &intensity_msg) {
        auto &color_message = dye_target_messages[static_cast<int>(color_target)];
        auto &intensity_message = dye_target_messages[static_cast<int>(intensity_target)];

        auto color_index = get_selected_index(color_target);
        auto intensity_index = get_selected_index(intensity_target);
        if (color_index != -1)
        {
            color_message = colors[color_index].color_block + color_msg;
            intensity_message = intensities[intensity_index].color_block + intensity_msg;
        }
        else
        {
            color_message = std::wstring{L"<IMG SRC='img://MENU_DummyTransparent.dds' WIDTH='12' "
                                         L"HEIGHT='1' HSPACE='0' VSPACE='-1'> "} +
                            color_msg;
            intensity_message = std::wstring{L"<IMG SRC='img://MENU_DummyTransparent.dds' "
                                             L"WIDTH='12' HEIGHT='1' HSPACE='0' VSPACE='-1'> "} +
                                intensity_msg;
        }
    };

    set_messages(dye_target_type::primary_color, dye_target_type::primary_intensity,
                 messages.primary_color, messages.primary_intensity);
    set_messages(dye_target_type::secondary_color, dye_target_type::secondary_intensity,
                 messages.secondary_color, messages.secondary_intensity);
    set_messages(dye_target_type::tertiary_color, dye_target_type::tertiary_intensity,
                 messages.tertiary_color, messages.tertiary_intensity);
}

int erdyes::local_player::get_selected_index(erdyes::dye_target_type dye_target)
{
    auto is_dye_target_color = is_color(dye_target);

    // Return the focused talkscript menu option if the dye target is currently being edited
    if (erdyes::get_talkscript_dye_target() == dye_target)
    {
        auto talkscript_focused_entry = erdyes::get_talkscript_focused_entry();

        // The minus 1 accounts for the "none" option in the color menu
        if (is_dye_target_color && is_valid_color_index(talkscript_focused_entry - 1))
            return talkscript_focused_entry - 1;
        else if (!is_dye_target_color && is_valid_intensity_index(talkscript_focused_entry))
            return talkscript_focused_entry;
    }

    auto world_chr_man = er::CS::WorldChrManImp::instance();
    if (!world_chr_man || !world_chr_man->main_player)
    {
        return -1;
    }

    auto equip_inventory_data =
        &world_chr_man->main_player->game_data->equip_game_data.equip_inventory_data;

    auto [base_goods_id, count] = get_dye_target_goods_range(dye_target);
    if (base_goods_id == -1)
    {
        return -1;
    }

    for (int i = 0; i < count; i++)
    {
        int goods_id = base_goods_id + i;
        int item_id = item_type_goods + goods_id;
        if (get_inventory_id(equip_inventory_data, &item_id) != -1)
        {
            return i;
        }
    }
    return is_dye_target_color ? default_color_index : default_intensity_index;
}

void erdyes::local_player::set_selected_index(erdyes::dye_target_type dye_target, int index)
{
    auto [base_goods_id, count] = get_dye_target_goods_range(dye_target);
    if (base_goods_id == -1)
    {
        return;
    }

    auto world_chr_man = er::CS::WorldChrManImp::instance();
    if (!world_chr_man || !world_chr_man->main_player)
    {
        return;
    }

    auto equip_inventory_data =
        &world_chr_man->main_player->game_data->equip_game_data.equip_inventory_data;

    // Remove any existing dummy items in the given range to clear out existing dye selections
    for (int i = 0; i < count; i++)
    {
        int goods_id = base_goods_id + i;
        int item_id = item_type_goods + goods_id;
        if (get_inventory_id(equip_inventory_data, &item_id) != -1)
        {
            add_remove_item(item_type_goods, goods_id, -1);
        }
    }

    // If a new selection was chosen, add an item to the player's inventory to store this selection
    if (index != -1)
        add_remove_item(item_type_goods, base_goods_id + index, 1);
    // If "none" was chosen for a color, also remove the item for the corresponding intensity
    else if (dye_target == erdyes::dye_target_type::primary_color)
        set_selected_index(erdyes::dye_target_type::primary_intensity, -1);
    else if (dye_target == erdyes::dye_target_type::secondary_color)
        set_selected_index(erdyes::dye_target_type::secondary_intensity, -1);
    else if (dye_target == erdyes::dye_target_type::tertiary_color)
        set_selected_index(erdyes::dye_target_type::tertiary_intensity, -1);
}
