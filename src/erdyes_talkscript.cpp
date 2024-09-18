/**
 * erdyes_talkscript.cpp
 *
 * Talkscript patching hook. This intercepts the start of Kalé's dialogue tree, and patches it to
 * include more options for modded shops.
 */
#include "erdyes_talkscript.hpp"
#include "erdyes_config.hpp"

#include <algorithm>
#include <array>
#include <spdlog/spdlog.h>

#include <elden-x/ezstate/ezstate.hpp>
#include <elden-x/ezstate/talk_commands.hpp>
#include <elden-x/utils/modutils.hpp>

#include "erdyes_messages.hpp"

enum class dye_target_type
{
    armor,
    horse,
};

enum class dye_color_type
{
    primary,
    secondary,
    tertiary,
};

// Stores the currently selected menu options, so we can reuse a single color ESD state and just
// keep track of which color we're updating here
static auto dye_target = dye_target_type::armor;
static auto dye_color = dye_color_type::primary;

typedef std::array<unsigned char, 6> int_value_data;

/**
 * Create an ESD expression representing a 4 byte integer
 */
static constexpr int_value_data make_int_value(int value)
{
    return {
        0x82,
        static_cast<unsigned char>((value & 0x000000ff)),
        static_cast<unsigned char>((value & 0x0000ff00) >> 8),
        static_cast<unsigned char>((value & 0x00ff0000) >> 16),
        static_cast<unsigned char>((value & 0xff000000) >> 24),
        0xa1,
    };
}

/**
 * Parse an ESD expression containing only a 1 or 4 byte integer
 */
static int get_ezstate_int_value(from::ezstate::expression &arg)
{
    // Single byte (plus final 0xa1) - used to store integers from -64 to 63
    if (arg.size() == 2)
    {
        return arg[0] - 64;
    }

    // Five bytes (plus final 0xa1) - used to store larger integers
    if (arg.size() == 6 && arg[0] == 0x82)
    {
        return *reinterpret_cast<int *>(&arg[1]);
    }

    return -1;
}

/**
 * Create an ESD expression that checks for GetTalkListEntryResult() returning a given value
 */
static constexpr std::array<unsigned char, 9> make_talk_list_result_evalutator(int value)
{
    return {
        0x57,
        0x84,
        0x82,
        static_cast<unsigned char>((value & 0x000000ff)),
        static_cast<unsigned char>((value & 0x0000ff00) >> 8),
        static_cast<unsigned char>((value & 0x00ff0000) >> 16),
        static_cast<unsigned char>((value & 0xff000000) >> 24),
        0x95,
        0xa1,
    };
}

/**
 * Common ESD expression (almost) always used as the third argument of AddTalkListData()
 */
static auto placeholder = make_int_value(-1);

/**
 * Common ESD expression passed as the argument to ShowGenericDialogShopMessage() for all purposes
 * in this mod
 */
static auto generic_dialog_shop_message = make_int_value(0);
static auto show_generic_dialog_shop_message_args = std::array{
    from::ezstate::expression(generic_dialog_shop_message),
};

/**
 * Common ESD expressions passed as the arguments for the "Cancel" talk list data option in several
 * locations
 */
static auto cancel_index = make_int_value(9999);
static auto cancel_message = make_int_value(erdyes::event_text_for_talk::cancel);
static auto cancel_args = std::array{
    from::ezstate::expression(cancel_index),
    from::ezstate::expression(cancel_message),
    from::ezstate::expression(placeholder),
};

/**
 * Common ESD expressions passed as the arguments for the "Back" talk list data option in several
 * locations
 */
static auto back_index = make_int_value(9999);
static auto back_message = make_int_value(erdyes::event_text_for_talk::back);
static auto back_args = std::array{
    from::ezstate::expression(back_index),
    from::ezstate::expression(back_message),
    from::ezstate::expression(placeholder),
};

/**
 * ESD state for selecting the dye target (armor or horse)
 */
static auto dye_target_armor_index = make_int_value(1);
static auto dye_target_armor_message = make_int_value(erdyes::event_text_for_talk::armor);
static auto dye_target_armor_args = std::array{
    from::ezstate::expression(dye_target_armor_index),
    from::ezstate::expression(dye_target_armor_message),
    from::ezstate::expression(placeholder),
};
static auto dye_target_horse_index = make_int_value(2);
static auto dye_target_horse_message = make_int_value(erdyes::event_text_for_talk::horse);
static auto dye_target_horse_args = std::array{
    from::ezstate::expression(dye_target_horse_index),
    from::ezstate::expression(dye_target_horse_message),
    from::ezstate::expression(placeholder),
};
static auto dye_target_events = std::array{
    from::ezstate::event(from::talk_command::close_shop_message),
    from::ezstate::event(from::talk_command::clear_talk_list_data),
    from::ezstate::event(from::talk_command::add_talk_list_data, dye_target_armor_args),
    from::ezstate::event(from::talk_command::add_talk_list_data, dye_target_horse_args),
    from::ezstate::event(from::talk_command::add_talk_list_data, cancel_args),
    from::ezstate::event(from::talk_command::show_shop_message,
                         show_generic_dialog_shop_message_args),
};
static auto dye_target_state = from::ezstate::state{
    .entry_events = dye_target_events,
};

/**
 * ESD state for selecting the color type (primary, secondary, or tertiary)
 */
static auto dye_color_type_primary_index = make_int_value(1);
static auto dye_color_type_primary_message =
    make_int_value(erdyes::event_text_for_talk::primary_color);
static auto dye_color_type_primary_args = std::array{
    from::ezstate::expression(dye_color_type_primary_index),
    from::ezstate::expression(dye_color_type_primary_message),
    from::ezstate::expression(placeholder),
};
static auto dye_color_type_secondary_index = make_int_value(2);
static auto dye_color_type_secondary_message =
    make_int_value(erdyes::event_text_for_talk::secondary_color);
static auto dye_color_type_secondary_args = std::array{
    from::ezstate::expression(dye_color_type_secondary_index),
    from::ezstate::expression(dye_color_type_secondary_message),
    from::ezstate::expression(placeholder),
};
static auto dye_color_type_tertiary_index = make_int_value(3);
static auto dye_color_type_tertiary_message =
    make_int_value(erdyes::event_text_for_talk::tertiary_color);
static auto dye_color_type_tertiary_args = std::array{
    from::ezstate::expression(dye_color_type_tertiary_index),
    from::ezstate::expression(dye_color_type_tertiary_message),
    from::ezstate::expression(placeholder),
};
static auto dye_color_type_events = std::array{
    from::ezstate::event(from::talk_command::close_shop_message),
    from::ezstate::event(from::talk_command::clear_talk_list_data),
    from::ezstate::event(from::talk_command::add_talk_list_data, dye_color_type_primary_args),
    from::ezstate::event(from::talk_command::add_talk_list_data, dye_color_type_secondary_args),
    from::ezstate::event(from::talk_command::add_talk_list_data, dye_color_type_tertiary_args),
    from::ezstate::event(from::talk_command::add_talk_list_data, cancel_args),
    from::ezstate::event(from::talk_command::show_shop_message,
                         show_generic_dialog_shop_message_args),
};
static auto dye_color_type_state = from::ezstate::state{
    .entry_events = dye_color_type_events,
};

/**
 * ESD state for selecting a color. This has a variable number of events, because colors can
 * be added in the .ini file.
 */
static auto color_state = from::ezstate::state();
static auto color_indexs = std::vector<int_value_data>();
static auto color_message_id_values = std::vector<int_value_data>();
static auto color_args = std::vector<std::array<from::ezstate::expression, 3>>();
static auto color_events = std::vector<from::ezstate::event>();

static auto apply_dyes_option_index = make_int_value(67);
static auto apply_dyes_message_id = make_int_value(erdyes::event_text_for_talk::apply_dyes);
static auto apply_dyes_args = std::array{
    from::ezstate::expression(apply_dyes_option_index),
    from::ezstate::expression(apply_dyes_message_id),
    from::ezstate::expression(placeholder),
};
static auto apply_dyes_evaluator = make_talk_list_result_evalutator(67);
static auto apply_dyes_transition = from::ezstate::transition{
    &color_state,
    apply_dyes_evaluator,
};

static void initialize_talkscript_states()
{
    color_indexs.reserve(erdyes::config::colors.size());
    color_message_id_values.reserve(erdyes::config::colors.size());
    color_args.reserve(erdyes::config::colors.size());
    color_events.reserve(erdyes::config::colors.size() + 4);
    color_events.emplace_back(from::talk_command::close_shop_message);
    color_events.emplace_back(from::talk_command::clear_talk_list_data);
    for (int i = 0; i < erdyes::config::colors.size(); i++)
    {
        auto &index = color_indexs.emplace_back(make_int_value(i));
        auto &message_id = color_message_id_values.emplace_back(
            make_int_value(erdyes::event_text_for_talk::dye_name_deselected_start + i));
        auto &args = color_args.emplace_back(std::array{
            from::ezstate::expression(index),
            from::ezstate::expression(message_id),
            from::ezstate::expression(placeholder),
        });
        color_events.emplace_back(from::talk_command::add_talk_list_data, args);
    }
    color_events.emplace_back(from::talk_command::add_talk_list_data, back_args);
    color_events.emplace_back(from::talk_command::show_shop_message,
                              show_generic_dialog_shop_message_args);
    color_state.entry_events = color_events;
}

static void update_talkscript_states()
{
    // TODO
    for (int i = 0; i < color_message_id_values.size(); i++)
    {
        color_message_id_values[i] =
            make_int_value(erdyes::event_text_for_talk::dye_name_deselected_start + i);
    }

    color_message_id_values[5] =
        make_int_value(erdyes::event_text_for_talk::dye_name_selected_start + 5);
}

static auto patched_events = std::array<from::ezstate::event, 100>{};
static auto patched_transitions = std::array<from::ezstate::transition *, 100>{};

/**
 * Return true if the given EzState transition goes to a state that opens the storage chest
 */
static bool is_sort_chest_transition(const from::ezstate::transition *transition)
{
    auto target_state = transition->target_state;
    return target_state != nullptr && !target_state->entry_events.empty() &&
           target_state->entry_events[0].command == from::talk_command::open_repository;
}

static bool patch_states(from::ezstate::state_group *state_group)
{
    from::ezstate::state *add_menu_state = nullptr;
    from::ezstate::state *menu_transition_state = nullptr;

    int event_index = -1;
    int transition_index = -1;

    // Look for a state that adds a "Sort chest" menu option, and a state that opens the storage
    // chest.
    for (auto &state : state_group->states)
    {
        for (int i = 0; i < state.entry_events.size(); i++)
        {
            auto &event = state.entry_events[i];
            if (event.command == from::talk_command::add_talk_list_data)
            {
                auto message_id = get_ezstate_int_value(event.args[1]);
                if (message_id == erdyes::event_text_for_talk::sort_chest)
                {
                    add_menu_state = &state;
                    event_index = i;
                }
                else if (message_id == erdyes::event_text_for_talk::apply_dyes)
                {
                    spdlog::debug("Not patching state group x{}, already patched",
                                  0x7fffffff - state_group->id);
                    return false;
                }
            }
        }

        for (int i = 0; i < state.transitions.size(); i++)
        {
            if (is_sort_chest_transition(state.transitions[i]))
            {
                menu_transition_state = &state;
                transition_index = i;
                break;
            }
        }
    }

    if (event_index == -1 || transition_index == -1)
    {
        return false;
    }

    spdlog::info("Patching state group x{}", 0x7fffffff - state_group->id);

    // Add an "Apply dyes" menu option
    auto &events = add_menu_state->entry_events;
    std::copy(events.begin(), events.end(), patched_events.begin());
    patched_events[events.size()] = {from::talk_command::add_talk_list_data, apply_dyes_args};
    events = {patched_events.data(), events.size() + 1};

    // Add a transition to the "Apply dyes" menu
    auto &transitions = menu_transition_state->transitions;
    std::copy(transitions.begin(), transitions.begin() + transition_index,
              patched_transitions.begin());
    std::copy(transitions.begin() + transition_index, transitions.end(),
              patched_transitions.begin() + transition_index + 1);
    patched_transitions[transition_index] = &apply_dyes_transition;
    transitions = {patched_transitions.data(), transitions.size() + 1};

    return true;
}

static void (*ezstate_enter_state)(from::ezstate::state *, from::ezstate::machine *, void *);

/**
 * Hook for EzState::state::Enter()
 */
static void ezstate_enter_state_detour(from::ezstate::state *state, from::ezstate::machine *machine,
                                       void *unk)
{
    if (state == machine->state_group->initial_state)
    {
        if (patch_states(machine->state_group))
        {
            // main_menu_return_transition.target_state = state;
        }
    }

    if (state == &color_state)
    {
        update_talkscript_states();
    }

    ezstate_enter_state(state, machine, unk);
}

void erdyes::setup_talkscript()
{
    initialize_talkscript_states();

    modutils::hook(
        {
            .aob = "80 7e 18 00"     // cmp byte ptr [rsi+0x18], 0
                   "74 15"           // je 27
                   "4c 8d 44 24 40"  // lea r8, [rsp+0x40]
                   "48 8b d6"        // mov rdx, rsi
                   "48 8b 4e 20"     // mov rcx, qword ptr [rsi+0x20]
                   "e8 ?? ?? ?? ??", // call EzState::state::Enter
            .offset = 18,
            .relative_offsets = {{1, 5}},
        },
        ezstate_enter_state_detour, ezstate_enter_state);
}
