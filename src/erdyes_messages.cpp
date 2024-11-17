/**
 * erdyes_messages.cpp
 *
 * New messages. This determines the message strings uses by the mod, and hooks the message lookup
 * function to return them.
 */
#include "erdyes_messages.hpp"
#include "erdyes_state.hpp"

#include <chrono>
#include <map>
#include <thread>

#include <elden-x/messages.hpp>
#include <elden-x/utils/localization.hpp>
#include <elden-x/utils/modutils.hpp>
#include <spdlog/spdlog.h>

erdyes::messages_type erdyes::messages;

static std::wstring apply_dyes_msg;
static std::wstring none_deselected_msg;
static std::wstring none_selected_msg;
static std::wstring back_msg;

static const wchar_t *(*msg_repository_lookup_entry)(from::CS::MsgRepositoryImp *, unsigned int,
                                                     from::msgbnd, int);

/**
 * Hook for MsgRepositoryImp::LookupEntry()
 *
 * Return menu text for the talkscript options and color names added by the mod, or fall back to
 * the default vanilla messages.
 */
static const wchar_t *msg_repository_lookup_entry_detour(from::CS::MsgRepositoryImp *msg_repository,
                                                         unsigned int unknown, from::msgbnd bnd_id,
                                                         int msg_id)
{
    if (bnd_id == from::msgbnd::event_text_for_talk &&
        msg_id >= erdyes::event_text_for_talk::mod_message_start &&
        msg_id < erdyes::event_text_for_talk::mod_message_end)
    {
        if (msg_id == erdyes::event_text_for_talk::apply_dyes)
        {
            return apply_dyes_msg.data();
        }
        else if (msg_id == erdyes::event_text_for_talk::primary_color)
        {
            auto i = static_cast<int>(erdyes::dye_target_type::primary_color);
            return erdyes::dye_target_messages[i].data();
        }
        else if (msg_id == erdyes::event_text_for_talk::secondary_color)
        {
            auto i = static_cast<int>(erdyes::dye_target_type::secondary_color);
            return erdyes::dye_target_messages[i].data();
        }
        else if (msg_id == erdyes::event_text_for_talk::tertiary_color)
        {
            auto i = static_cast<int>(erdyes::dye_target_type::tertiary_color);
            return erdyes::dye_target_messages[i].data();
        }
        else if (msg_id == erdyes::event_text_for_talk::primary_intensity)
        {
            auto i = static_cast<int>(erdyes::dye_target_type::primary_intensity);
            return erdyes::dye_target_messages[i].data();
        }
        else if (msg_id == erdyes::event_text_for_talk::secondary_intensity)
        {
            auto i = static_cast<int>(erdyes::dye_target_type::secondary_intensity);
            return erdyes::dye_target_messages[i].data();
        }
        else if (msg_id == erdyes::event_text_for_talk::tertiary_intensity)
        {
            auto i = static_cast<int>(erdyes::dye_target_type::tertiary_intensity);
            return erdyes::dye_target_messages[i].data();
        }
        else if (msg_id == erdyes::event_text_for_talk::none_deselected)
        {
            return none_deselected_msg.data();
        }
        else if (msg_id == erdyes::event_text_for_talk::none_selected)
        {
            return none_selected_msg.data();
        }
        else if (msg_id == erdyes::event_text_for_talk::back)
        {
            return back_msg.data();
        }
        else if (msg_id >= erdyes::event_text_for_talk::dye_color_selected_start &&
                 msg_id <
                     erdyes::event_text_for_talk::dye_color_selected_start + erdyes::colors.size())
        {
            auto color_index = msg_id - erdyes::event_text_for_talk::dye_color_selected_start;
            return erdyes::colors[color_index].selected_message.data();
        }
        else if (msg_id >= erdyes::event_text_for_talk::dye_color_deselected_start &&
                 msg_id < erdyes::event_text_for_talk::dye_color_deselected_start +
                              erdyes::colors.size())
        {
            auto color_index = msg_id - erdyes::event_text_for_talk::dye_color_deselected_start;
            return erdyes::colors[color_index].deselected_message.data();
        }
        else if (msg_id >= erdyes::event_text_for_talk::dye_intensity_selected_start &&
                 msg_id < erdyes::event_text_for_talk::dye_intensity_selected_start +
                              erdyes::intensities.size())
        {
            auto intensity_index =
                msg_id - erdyes::event_text_for_talk::dye_intensity_selected_start;
            return erdyes::intensities[intensity_index].selected_message.data();
        }
        else if (msg_id >= erdyes::event_text_for_talk::dye_intensity_deselected_start &&
                 msg_id < erdyes::event_text_for_talk::dye_intensity_deselected_start +
                              erdyes::intensities.size())
        {
            auto intensity_index =
                msg_id - erdyes::event_text_for_talk::dye_intensity_deselected_start;
            return erdyes::intensities[intensity_index].deselected_message.data();
        }
    }

    return msg_repository_lookup_entry(msg_repository, unknown, bnd_id, msg_id);
}

void erdyes::setup_messages()
{
    spdlog::info("Waiting for messages...");
    while (!from::CS::MsgRepositoryImp::instance())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Hook MsgRepositoryImp::LookupEntry() to return messages added by the mod
    modutils::hook(
        {
            .aob = "8b da"        // mov ebx, edx
                   "44 8b ca"     // mov r9d, edx
                   "33 d2"        // xor edx, edx
                   "48 8b f9"     // mov rdi, rcx
                   "44 8d 42 6f", // lea r8d, [rdx+0x6f]
            .offset = 14,
            .relative_offsets = {{1, 5}},
        },
        msg_repository_lookup_entry_detour, msg_repository_lookup_entry);

    // Pick the messages to use based on the player's selected language for the game in Steam
    auto language = get_steam_language();

    auto localized_messages = messages_by_lang.find(language);
    if (localized_messages != messages_by_lang.end())
    {
        spdlog::info("Detected language \"{}\"", language);
        messages = localized_messages->second;
    }
    else
    {
        spdlog::warn("Unknown language \"{}\", defaulting to English", language);
        messages = messages_by_lang.at("english");
    }

    apply_dyes_msg = messages.apply_dyes;

    // Detect if a right-to-left language is being used, since Elden Ring's poor text shaping
    // support requires us to change the order of some messages
    bool is_rtl = language == "arabic";

    // Detect if the ELDEN RING: Reforged mod is running, since some adjustments to menu text
    // are needed
    auto calibrations_ver = std::wstring_view{msg_repository_lookup_entry(
        from::CS::MsgRepositoryImp::instance(), 0, from::msgbnd::menu_text, 401322)};
    bool is_reforged = calibrations_ver.find(L"ELDEN RING Reforged") != std::wstring::npos;

    none_deselected_msg = format_option_message(L" " + messages.none, false, is_rtl);
    none_selected_msg = format_option_message(L" " + messages.none, true, is_rtl);

    const std::wstring back_spacer = L"<IMG SRC='img://MENU_DummyTransparent.dds' WIDTH='32' "
                                     L"HEIGHT='2' HSPACE='0' VSPACE='-1'>";
    if (is_rtl)
        back_msg = messages.back + L" " + back_spacer;
    else
        back_msg = back_spacer + L" " + messages.back;

    // Add an icon to the "Apply dyes" option to align with the other menu items in Reforged
    if (is_reforged)
    {
        const std::wstring reforged_icon =
            L"<IMG SRC='img://SB_ERR_Body_Hues.png' WIDTH='32' HEIGHT='32' VSPACE='-16'>";
        if (is_rtl)
            apply_dyes_msg = apply_dyes_msg + L" " + reforged_icon;
        else
            apply_dyes_msg = reforged_icon + L" " + apply_dyes_msg;
    }
}

std::wstring erdyes::format_option_message(std::wstring const &label, bool selected, bool rtl)
{
    auto img_src = selected ? L"MENU_Lockon_01a.png" : L"MENU_DummyTransparent.dds";
    auto icon = std::wstring{L"<IMG SRC='img://"} + img_src +
                L"' WIDTH='20' HEIGHT='20' HSPACE='0' VSPACE='-1'>";
    return rtl ? (label + icon) : (icon + label);
}