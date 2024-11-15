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

std::wstring none_deselected_msg;
std::wstring none_selected_msg;
std::wstring back_msg;

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
            return erdyes::messages.apply_dyes.data();
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

    // Reverse the order of the images and text for RTL. Elden Ring doesn't support bidirectional
    // text (I think), so we have to manually change the order to the right direction.
    if (language == "arabic")
    {
        none_deselected_msg =
            messages.none +
            L"<IMG SRC='img://MENU_DummyTransparent.dds' WIDTH='32' HEIGHT='1' HSPACE='0'"
            L" VSPACE='-1'> ";

        none_selected_msg =
            messages.none +
            L"<IMG SRC='img://MENU_DummyTransparent.dds' WIDTH='12' HEIGHT='1' HSPACE='0' "
            L"VSPACE='-1'> " +
            L"<IMG SRC='img://MENU_Lockon_01a.png' WIDTH='20' HEIGHT='20' HSPACE='0' VSPACE='-1'>";

        back_msg = messages.back +
                   L" <IMG SRC='img://MENU_DummyTransparent.dds' WIDTH='32' HEIGHT='1' HSPACE='0' "
                   L"VSPACE='-1'>";
    }
    else
    {
        none_deselected_msg =
            L"<IMG SRC='img://MENU_DummyTransparent.dds' WIDTH='32' HEIGHT='1' HSPACE='0'"
            L" VSPACE='-1'> " +
            messages.none;

        none_selected_msg =
            L"<IMG SRC='img://MENU_Lockon_01a.png' WIDTH='20' HEIGHT='20' HSPACE='0' VSPACE='-1'>"
            L"<IMG SRC='img://MENU_DummyTransparent.dds' WIDTH='12' HEIGHT='1' HSPACE='0' "
            L"VSPACE='-1'> " +
            messages.none;

        back_msg = L"<IMG SRC='img://MENU_DummyTransparent.dds' WIDTH='32' HEIGHT='1' HSPACE='0' "
                   L"VSPACE='-1'> " +
                   messages.back;
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
}
