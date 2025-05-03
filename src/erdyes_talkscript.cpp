/**
 * erdyes_talkscript.cpp
 *
 * Talkscript patching hook. This intercepts the start of Site of Grace dialogue tree, and patches
 * it to include more options for applying armor dyes. This is verbose and repetitive, but it
 * allows the DLL to add a menu without merging files, and for color options to be dynamically
 * generated from the .ini config.
 */
#include "erdyes_talkscript.hpp"
#include "erdyes_local_player.hpp"
#include "erdyes_messages.hpp"
#include "talkscript_utils.hpp"

#include <array>
#include <spdlog/spdlog.h>
#include <vector>

#include <elden-x/ezstate/ezstate.hpp>
#include <elden-x/ezstate/talk_commands.hpp>
#include <elden-x/menu/generic_list_select_dialog.hpp>
#include <elden-x/menu/menu_man.hpp>
#include <elden-x/utils/modutils.hpp>

// Stores the current color option being edited
static erdyes::dye_target_type talkscript_dye_target{erdyes::dye_target_type::none};

// Talkscript for selecting a color or none
static auto color_menu = talkscript_menu{};
static auto color_selected_return_transition =
    er::ezstate::transition{nullptr, er::ezstate::expression{true_expression}};
static auto color_selected_return_transitions = std::array{&color_selected_return_transition};
static auto color_selected_states = std::vector<er::ezstate::state>{};
static auto color_none_selected_state =
    er::ezstate::state{.transitions = er::ezstate::transitions{color_selected_return_transitions}};

// Talkscript for selecting an intensity level
static auto intensity_menu = talkscript_menu{};
static auto intensity_selected_return_transition =
    er::ezstate::transition{nullptr, er::ezstate::expression{true_expression}};
static auto intensity_selected_return_transitions =
    std::array{&intensity_selected_return_transition};
static auto intensity_selected_states = std::vector<er::ezstate::state>{};

// Talkscript for selecting the color type (primary, secondary, or tertiary) and what to change
// (color or intensity)
static auto dye_target_color_successor_transition =
    er::ezstate::transition{&color_menu.state, er::ezstate::expression{true_expression}};
static auto dye_target_color_successor_transitions =
    std::array{&dye_target_color_successor_transition};
static auto dye_target_intensity_successor_transition =
    er::ezstate::transition{&intensity_menu.state, er::ezstate::expression{true_expression}};
static auto dye_target_intensity_successor_transitions =
    std::array{&dye_target_intensity_successor_transition};
static auto primary_color_state = er::ezstate::state{
    .transitions = er::ezstate::transitions{dye_target_color_successor_transitions}};
static auto secondary_color_state = er::ezstate::state{
    .transitions = er::ezstate::transitions{dye_target_color_successor_transitions}};
static auto tertiary_color_state = er::ezstate::state{
    .transitions = er::ezstate::transitions{dye_target_color_successor_transitions}};
static auto primary_intensity_state = er::ezstate::state{
    .transitions = er::ezstate::transitions{dye_target_intensity_successor_transitions}};
static auto secondary_intensity_state = er::ezstate::state{
    .transitions = er::ezstate::transitions{dye_target_intensity_successor_transitions}};
static auto tertiary_intensity_state = er::ezstate::state{
    .transitions = er::ezstate::transitions{dye_target_intensity_successor_transitions}};
static auto dye_target_menu = talkscript_menu{{
    {1, erdyes::event_text_for_talk::primary_color, &primary_color_state},
    {2, erdyes::event_text_for_talk::secondary_color, &secondary_color_state},
    {3, erdyes::event_text_for_talk::tertiary_color, &tertiary_color_state},
    {4, erdyes::event_text_for_talk::primary_intensity, &primary_intensity_state},
    {5, erdyes::event_text_for_talk::secondary_intensity, &secondary_intensity_state},
    {6, erdyes::event_text_for_talk::tertiary_intensity, &tertiary_intensity_state},
    {99, erdyes::event_text_for_talk::cancel, nullptr, true},
}};

// Talkscript entry for entering the dyes menu from the Site of Grace main menu
static auto apply_dyes_opt =
    talkscript_menu_option{67, erdyes::event_text_for_talk::apply_dyes, &dye_target_menu.state};

static void initialize_talkscript_states()
{
    color_selected_states.clear();
    color_selected_states.reserve(erdyes::colors.size());

    std::vector<talkscript_menu_option> color_opts;
    color_opts.reserve(erdyes::colors.size() + 2);
    color_opts.emplace_back(999998, erdyes::event_text_for_talk::none_selected,
                            &color_none_selected_state);
    for (int i = 0; i < erdyes::colors.size(); i++)
    {
        auto &successor_state = color_selected_states.emplace_back(
            0, er::ezstate::transitions{color_selected_return_transitions});

        color_opts.emplace_back(i + 1, erdyes::event_text_for_talk::dye_color_deselected_start + i,
                                &successor_state);
    }
    color_opts.emplace_back(999999, erdyes::event_text_for_talk::back, &dye_target_menu.state,
                            true);
    color_menu.set_opts(color_opts);

    intensity_selected_states.clear();
    intensity_selected_states.reserve(erdyes::intensities.size());

    std::vector<talkscript_menu_option> intensity_opts;
    intensity_opts.reserve(erdyes::intensities.size() + 1);
    for (int i = 0; i < erdyes::intensities.size(); i++)
    {
        auto &successor_state = intensity_selected_states.emplace_back(
            0, er::ezstate::transitions{intensity_selected_return_transitions});

        intensity_opts.emplace_back(i + 1,
                                    erdyes::event_text_for_talk::dye_intensity_deselected_start + i,
                                    &successor_state);
    }
    intensity_opts.emplace_back(999999, erdyes::event_text_for_talk::back, &dye_target_menu.state,
                                true);
    intensity_menu.set_opts(intensity_opts);
}

static auto patched_events = std::array<er::ezstate::event, 100>{};
static auto patched_transitions = std::array<er::ezstate::transition *, 100>{};

/**
 * Return true if the given EzState event is the given menu option
 */
static bool is_talk_list_data_event(er::ezstate::event &event, int expected_message_id)
{
    if (event.command == er::talk_command::add_talk_list_data)
    {
        auto message_id = get_ezstate_int_value(event.args[1]);
        return message_id == expected_message_id;
    }

    if (event.command == er::talk_command::add_talk_list_data_if ||
        event.command == er::talk_command::add_talk_list_data_alt)
    {
        auto message_id = get_ezstate_int_value(event.args[2]);
        return message_id == expected_message_id;
    }

    return false;
}

/**
 * Return true if the given EzState transition goes to a state that opens the storage chest
 */
static bool is_sort_chest_transition(const er::ezstate::transition *transition)
{
    auto target_state = transition->target_state;
    return target_state != nullptr && !target_state->entry_events.empty() &&
           target_state->entry_events[0].command == er::talk_command::open_repository;
}

static bool is_grace_state_group(er::ezstate::state_group *state_group)
{
    for (auto &state : state_group->states)
    {
        for (auto &event : state.entry_events)
        {
            // Exclude the training grounds statue in The Convergence, which has a "Sort Chest"
            // menu option but shouldn't include the dye menu.
            if (is_talk_list_data_event(
                    event, erdyes::event_text_for_talk::convergence_training_grounds_settings))
            {
                return false;
            }
            if (is_talk_list_data_event(event, erdyes::event_text_for_talk::sort_chest))
            {
                return true;
            }
        }
    }
    return false;
}

static bool patch_state_group(er::ezstate::state_group *state_group)
{
    er::ezstate::state *add_menu_state = nullptr;
    er::ezstate::state *menu_transition_state = nullptr;

    int event_index = -1;
    int transition_index = -1;

    // Look for a state that adds a "Sort chest" menu option, and a state that opens the storage
    // chest.
    for (auto &state : state_group->states)
    {
        for (int i = 0; i < state.entry_events.size(); i++)
        {
            auto &event = state.entry_events[i];
            if (is_talk_list_data_event(event, erdyes::event_text_for_talk::sort_chest))
            {
                add_menu_state = &state;
                event_index = i;
            }
            else if (event.command == er::talk_command::add_talk_list_data)
            {
                auto message_id = get_ezstate_int_value(event.args[1]);
                if (message_id == erdyes::event_text_for_talk::apply_dyes)
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
    patched_events[events.size()] = {er::talk_command::add_talk_list_data, apply_dyes_opt.args};
    events = {patched_events.data(), events.size() + 1};

    // Add a transition to the "Apply dyes" menu
    auto &transitions = menu_transition_state->transitions;
    std::copy(transitions.begin(), transitions.begin() + transition_index,
              patched_transitions.begin());
    std::copy(transitions.begin() + transition_index, transitions.end(),
              patched_transitions.begin() + transition_index + 1);
    patched_transitions[transition_index] = &apply_dyes_opt.transition;
    transitions = {patched_transitions.data(), transitions.size() + 1};

    return true;
}

/**
 * Triggers custom mod behavior when entering a patched talkscript state, returning the new dye
 * target
 */
static void handle_dye_states(er::ezstate::state *state)
{
    // Update the messages for the color picker dialog to show a dot next to the selected color
    auto update_color_messages = [](erdyes::dye_target_type dye_target) {
        int selected_index = erdyes::local_player::get_selected_index(dye_target);
        for (int i = 0; i < color_menu.opts.size() - 1; i++)
        {
            int message_id;
            if (i == 0)
            {
                message_id = selected_index == -1 ? erdyes::event_text_for_talk::none_selected
                                                  : erdyes::event_text_for_talk::none_deselected;
            }
            else
            {
                auto color_index = i - 1;
                message_id = (selected_index == color_index
                                  ? erdyes::event_text_for_talk::dye_color_selected_start
                                  : erdyes::event_text_for_talk::dye_color_deselected_start) +
                             color_index;
            }

            color_menu.opts[i].message = make_int_expression(message_id);
        }
        talkscript_dye_target = dye_target;
    };

    // Update the messages for the intensity picker dialog to show a dot next to the selected color
    auto update_intensity_messages = [](erdyes::dye_target_type dye_target) {
        int selected_index = erdyes::local_player::get_selected_index(dye_target);
        for (int i = 0; i < intensity_menu.opts.size() - 1; i++)
        {
            int message_id = (selected_index == i
                                  ? erdyes::event_text_for_talk::dye_intensity_selected_start
                                  : erdyes::event_text_for_talk::dye_intensity_deselected_start) +
                             i;
            intensity_menu.opts[i].message = make_int_expression(message_id);
        }
        talkscript_dye_target = dye_target;
    };

    if (state == &dye_target_menu.state)
    {
        talkscript_dye_target = erdyes::dye_target_type::none;
        erdyes::local_player::update_dye_target_messages();
    }
    // When one of the six options is chosen, store the selection and update the list of options
    // to include a dot next to the currently selected one
    else if (state == &primary_color_state)
    {
        update_color_messages(erdyes::dye_target_type::primary_color);
    }
    else if (state == &secondary_color_state)
    {
        update_color_messages(erdyes::dye_target_type::secondary_color);
    }
    else if (state == &tertiary_color_state)
    {
        update_color_messages(erdyes::dye_target_type::tertiary_color);
    }
    else if (state == &primary_intensity_state)
    {
        update_intensity_messages(erdyes::dye_target_type::primary_intensity);
    }
    else if (state == &secondary_intensity_state)
    {
        update_intensity_messages(erdyes::dye_target_type::secondary_intensity);
    }
    else if (state == &tertiary_intensity_state)
    {
        update_intensity_messages(erdyes::dye_target_type::tertiary_intensity);
    }
    // Unset the current color if "none" is selected in the color picker
    else if (state == &color_none_selected_state)
    {
        erdyes::local_player::set_selected_index(talkscript_dye_target, -1);
        talkscript_dye_target = erdyes::dye_target_type::none;
    }
    else
    {
        // Set the current color or intensity if a selection is made in the color or intensity
        // picker
        for (int i = 0; i < color_selected_states.size(); i++)
        {
            if (state == &color_selected_states[i])
            {
                erdyes::local_player::set_selected_index(talkscript_dye_target, i);
                break;
            }
        }
        for (int i = 0; i < intensity_selected_states.size(); i++)
        {
            if (state == &intensity_selected_states[i])
            {
                erdyes::local_player::set_selected_index(talkscript_dye_target, i);
                break;
            }
        }

        // For any other state not added by the mod, reset the dye target to none
        if (state != &color_menu.state && state != &color_menu.branch_state &&
            state != &intensity_menu.state && state != &intensity_menu.branch_state)
        {
            talkscript_dye_target = erdyes::dye_target_type::none;
        }
    }
}

static void (*ezstate_enter_state)(er::ezstate::state *, er::ezstate::machine *, void *);

/**
 * Hook for EzState::state::Enter()
 */
static void ezstate_enter_state_detour(er::ezstate::state *state, er::ezstate::machine *machine,
                                       void *unk)
{
    if (is_grace_state_group(machine->state_group))
    {
        if (state == machine->state_group->initial_state && patch_state_group(machine->state_group))
        {
            dye_target_menu.opts.back().transition.target_state =
                machine->state_group->initial_state;

            color_selected_return_transition.target_state = &dye_target_menu.state;
            intensity_selected_return_transition.target_state = &dye_target_menu.state;
        }

        handle_dye_states(state);
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

int erdyes::get_talkscript_focused_entry()
{
    auto menu_man = er::CS::CSMenuManImp::instance();
    if (menu_man && menu_man->popup_menu && menu_man->popup_menu->window)
    {
        auto dialog =
            reinterpret_cast<er::CS::GenericListSelectDialog *>(menu_man->popup_menu->window);

        if (dialog->grid_control.current_item >= 0 &&
            dialog->grid_control.current_item < dialog->grid_control.item_count)
        {
            return dialog->grid_control.current_item;
        }
    }

    return -1;
}

erdyes::dye_target_type erdyes::get_talkscript_dye_target()
{
    return talkscript_dye_target;
}