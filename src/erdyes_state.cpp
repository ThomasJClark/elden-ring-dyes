/**
 * erdyes_player_state.hpp
 *
 * Keeps track of the current dye selections for the player, saving and loading them and exposing
 * the results to the talkscript, messages, and color application systems.
 */
#include "erdyes_state.hpp"
#include "erdyes_messages.hpp"

#include <elden-x/chr/world_chr_man.hpp>
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

std::array<std::wstring, 6> erdyes::dye_target_messages;

std::vector<erdyes::color> erdyes::colors;
std::vector<erdyes::intensity> erdyes::intensities;

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
static constexpr std::wstring format_option_message(std::wstring const &color_block,
                                                    std::wstring const &name, bool selected)
{
    auto img_src = selected ? L"MENU_Lockon_01a.png" : L"MENU_DummyTransparent.dds";
    return std::wstring{L"<IMG SRC='img://"} + img_src +
           L"' WIDTH='20' HEIGHT='20' HSPACE='0' VSPACE='-1'>" + color_block + name;
};

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

void erdyes::add_color_option(const std::wstring &name, const std::wstring &hex_code, float r,
                              float g, float b)
{
    auto color_block = format_color_block(hex_code);
    colors.emplace_back(color_block, format_option_message(color_block, name, true),
                        format_option_message(color_block, name, false), r, g, b);
}

void erdyes::add_intensity_option(const std::wstring &name, const std::wstring &hex_code, float i)
{
    auto color_block = format_color_block(hex_code);
    intensities.emplace_back(color_block, format_option_message(color_block, name, true),
                             format_option_message(color_block, name, false), i);
}

void erdyes::update_dye_target_messages()
{
    auto set_messages = [](dye_target_type color_target, dye_target_type intensity_target,
                           const std::wstring &color_msg, const std::wstring &intensity_msg) {
        auto &color_message = dye_target_messages[static_cast<int>(color_target)];
        auto &intensity_message = dye_target_messages[static_cast<int>(intensity_target)];

        auto color_index = get_selected_option(color_target);
        auto intensity_index = get_selected_option(intensity_target);
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

int erdyes::get_selected_option(erdyes::dye_target_type dye_target)
{
    auto world_chr_man = from::CS::WorldChrManImp::instance();
    if (!world_chr_man || !world_chr_man->main_player)
    {
        return -1;
    }

    auto equip_inventory_data =
        &world_chr_man->main_player->player_game_data->equip_game_data.equip_inventory_data;

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
    return is_color(dye_target) ? default_color_index : default_intensity_index;
}

void erdyes::set_selected_option(erdyes::dye_target_type dye_target, int index)
{
    auto [base_goods_id, count] = get_dye_target_goods_range(dye_target);
    if (base_goods_id == -1)
    {
        return;
    }

    auto world_chr_man = from::CS::WorldChrManImp::instance();
    if (!world_chr_man || !world_chr_man->main_player)
    {
        return;
    }

    auto equip_inventory_data =
        &world_chr_man->main_player->player_game_data->equip_game_data.equip_inventory_data;

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
    {
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