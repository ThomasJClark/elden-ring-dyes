/**
 * erdyes_messages.cpp
 *
 * New messages. This determines the message strings uses by the mod, and hooks the message lookup
 * function to return them.
 */
#include "erdyes_messages.hpp"
#include "erdyes_colors.hpp"

#include <chrono>
#include <map>
#include <thread>

#include <elden-x/messages.hpp>
#include <elden-x/utils/localization.hpp>
#include <elden-x/utils/modutils.hpp>
#include <spdlog/spdlog.h>

// static const std::map<int, const std::wstring> *mod_event_text_for_talk;

static from::CS::MsgRepositoryImp *msg_repository = nullptr;

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
            return L"Apply dyes"; // TODO: intl
        }
        else if (msg_id == erdyes::event_text_for_talk::primary_color)
        {
            return L"Primary color"; // TODO: intl
        }
        else if (msg_id == erdyes::event_text_for_talk::secondary_color)
        {
            return L"Secondary color"; // TODO: intl
        }
        else if (msg_id == erdyes::event_text_for_talk::tertiary_color)
        {
            return L"Tertiary color"; // TODO: intl
        }
        else if (msg_id == erdyes::event_text_for_talk::none_deselected)
        {
            return L"<IMG SRC='img://MENU_DummyTransparent.dds' WIDTH='32' HEIGHT='1' HSPACE='0' "
                   L"VSPACE='-1'> "
                   L"None"; // TODO: intl
        }
        else if (msg_id == erdyes::event_text_for_talk::none_selected)
        {
            return "<IMG SRC='img://MENU_Lockon_01a.png' WIDTH='20' HEIGHT='20' HSPACE='0' "
                   "VSPACE='-1'>"
                   L"<IMG SRC='img://MENU_DummyTransparent.dds' WIDTH='12' HEIGHT='1' HSPACE='0' "
                   L"VSPACE='-1'> "
                   L"None"; // TODO: intl
        }
        else if (msg_id == erdyes::event_text_for_talk::back)
        {
            return L"<IMG SRC='img://MENU_DummyTransparent.dds' WIDTH='32' HEIGHT='1' HSPACE='0' "
                   L"VSPACE='-1'> "
                   L"Back"; // TODO: intl
        }
        else if (msg_id >= erdyes::event_text_for_talk::dye_name_selected_start &&
                 msg_id <
                     erdyes::event_text_for_talk::dye_name_selected_start + erdyes::colors.size())
        {
            auto color_index = msg_id - erdyes::event_text_for_talk::dye_name_selected_start;
            return erdyes::colors[color_index].selected_message.data();
        }
        else if (msg_id >= erdyes::event_text_for_talk::dye_name_deselected_start &&
                 msg_id <
                     erdyes::event_text_for_talk::dye_name_deselected_start + erdyes::colors.size())
        {
            auto color_index = msg_id - erdyes::event_text_for_talk::dye_name_deselected_start;
            return erdyes::colors[color_index].deselected_message.data();
        }
    }

    return msg_repository_lookup_entry(msg_repository, unknown, bnd_id, msg_id);
}

void erdyes::setup_messages()
{
    // Pick the messages to use based on the player's selected language for the game in Steam
    auto language = get_steam_language();
    // auto localized_messages = event_text_for_talk_by_lang.find(language);
    // if (localized_messages != event_text_for_talk_by_lang.end())
    // {
    //     spdlog::info("Detected language \"{}\"", language);
    //     mod_event_text_for_talk = &localized_messages->second;
    // }
    // else
    // {
    //     spdlog::warn("Unknown language \"{}\", defaulting to English", language);
    //     mod_event_text_for_talk = &event_text_for_talk_by_lang.at("english");
    // }

    auto msg_repository_address = modutils::scan<from::CS::MsgRepositoryImp *>({
        .aob = "48 8B 3D ?? ?? ?? ?? 44 0F B6 30 48 85 FF 75",
        .relative_offsets = {{3, 7}},
    });

    while (!(msg_repository = *msg_repository_address))
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
}
