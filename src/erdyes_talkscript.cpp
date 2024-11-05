/**
 * erdyes_talkscript.cpp
 *
 * Talkscript patching hook. This intercepts the start of Kalé's dialogue tree, and patches it to
 * include more options for modded shops.
 */
#include "erdyes_talkscript.hpp"
#include "erdyes_messages.hpp"
#include "erdyes_state.hpp"

#include <algorithm>
#include <array>
#include <spdlog/spdlog.h>

#include <elden-x/ezstate/ezstate.hpp>
#include <elden-x/ezstate/talk_commands.hpp>
#include <elden-x/menu/generic_list_select_dialog.hpp>
#include <elden-x/menu/menu_man.hpp>
#include <elden-x/utils/modutils.hpp>

erdyes::talkscript::dye_target_type erdyes::talkscript::dye_target =
    erdyes::talkscript::dye_target_type::none;

typedef std::array<unsigned char, 6> int_expression;

/**
 * Create an ESD expression representing a 4 byte integer
 */
static constexpr int_expression make_int_expression(int value)
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
static constexpr std::array<unsigned char, 9> make_talk_list_result_expression(int value)
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
static auto placeholder_expression = make_int_expression(-1);

/**
 * Common ESD expression passed as the argument to ShowGenericDialogShopMessage() for all purposes
 * in this mod
 */
static auto generic_dialog_shop_message = make_int_expression(0);
static auto show_generic_dialog_shop_message_args = std::array{
    from::ezstate::expression(generic_dialog_shop_message),
};

/**
 * Common ESD expressions passed as the arguments for the "Cancel" talk list data option in several
 * locations
 */
static auto cancel_index = make_int_expression(99999);
static auto cancel_message = make_int_expression(erdyes::event_text_for_talk::cancel);
static auto cancel_args = std::array{
    from::ezstate::expression(cancel_index),
    from::ezstate::expression(cancel_message),
    from::ezstate::expression(placeholder_expression),
};

/**
 * Common ESD expression that evaluates to true for the final "else" transition
 */
static auto true_expression = std::array<unsigned char, 2>{0x41, 0xa1};

/**
 * Common ESD expression for the generic talk menu closing
 */
static auto talk_menu_closed_expression =
    std::array<unsigned char, 40>{// start?
                                  (unsigned char)0x7b,
                                  // CheckSpecificPersonMenuIsOpen(1, 0)
                                  0x82, 0x01, 0x00, 0x00, 0x00, 0x82, 0x00, 0x00, 0x00, 0x00, 0x86,
                                  // 1
                                  0x82, 0x01, 0x00, 0x00, 0x00,
                                  // ==
                                  0x95,
                                  // CheckSpecificPersonGenericDialogIsOpen(0)
                                  0x7a, 0x82, 0x00, 0x00, 0x00, 0x00, 0x85,
                                  // 0
                                  0x82, 0x00, 0x00, 0x00, 0x00,
                                  // ==
                                  0x95,
                                  // &&
                                  0x98,
                                  // 0
                                  0x82, 0x00, 0x00, 0x00, 0x00,
                                  // ==
                                  0x95,
                                  // end
                                  0xa1};

/**
 * Common ESD expressions passed as the arguments for the "Back" talk list data option in several
 * locations
 */
static auto back_index = make_int_expression(99999);
static auto back_message = make_int_expression(erdyes::event_text_for_talk::back);
static auto back_args = std::array{
    from::ezstate::expression(back_index),
    from::ezstate::expression(back_message),
    from::ezstate::expression(placeholder_expression),
};

/**
 * Helper that holds commonly used structs for a AddTalkListData() call
 */
class talkscript_menu_option
{
  public:
    std::array<unsigned char, 6> index;
    std::array<unsigned char, 6> message;
    std::array<unsigned char, 9> condition;
    std::array<from::ezstate::expression, 3> args;
    from::ezstate::transition transition;

    talkscript_menu_option(int index, int message_id, from::ezstate::state *target_state)
        : index(make_int_expression(index)), message(make_int_expression(message_id)),
          condition(make_talk_list_result_expression(index)),
          args{this->index, this->message, placeholder_expression},
          transition(target_state, this->condition)
    {
    }
};

/**
 * Talkscript for selecting a color. This has a variable number of events, because colors can
 * be added in the .ini file.
 */
static auto color_branch_transitions = std::vector<from::ezstate::transition *>{};
static auto color_branch_back_transition =
    from::ezstate::transition{nullptr, from::ezstate::expression{true_expression}};
static auto color_branch_state = from::ezstate::state{};

static auto color_events = std::vector<from::ezstate::event>{};
static auto color_transition = from::ezstate::transition{
    &color_branch_state, from::ezstate::expression{talk_menu_closed_expression}};
static auto color_transitions = std::array{&color_transition};
static auto color_state =
    from::ezstate::state{.transitions = from::ezstate::transitions{color_transitions}};

/**
 * Successor ESD states for each color option. These do nothing and immediately return to the color
 * selector state.
 */
static auto color_selected_return_transition =
    from::ezstate::transition{nullptr, from::ezstate::expression{true_expression}};
static auto color_selected_return_transitions = std::array{&color_selected_return_transition};
static auto color_selected_states = std::vector<from::ezstate::state>{};
static auto color_none_selected_state = from::ezstate::state{
    .transitions = from::ezstate::transitions{color_selected_return_transitions}};

static auto color_opts = std::vector<talkscript_menu_option>{};
static auto color_none_opt = talkscript_menu_option{
    99998, erdyes::event_text_for_talk::none_selected, &color_none_selected_state};

/**
 * Talkscript for selecting the color type (primary, secondary, or tertiary) and what to change
 * (color or intensity)
 */
static auto dye_target_successor_transition =
    from::ezstate::transition{&color_state, from::ezstate::expression{true_expression}};
static auto dye_target_successor_transitions = std::array{&dye_target_successor_transition};

// Dummy states that each option passes through before going to the color picker state. The hook
// determines which option is being picked based on which of these states we went through.
static auto primary_state = from::ezstate::state{
    .transitions = from::ezstate::transitions{dye_target_successor_transitions}};
static auto secondary_state = from::ezstate::state{
    .transitions = from::ezstate::transitions{dye_target_successor_transitions}};
static auto tertiary_state = from::ezstate::state{
    .transitions = from::ezstate::transitions{dye_target_successor_transitions}};

static auto dye_target_primary_opt =
    talkscript_menu_option{1, erdyes::event_text_for_talk::primary_color, &primary_state};
static auto dye_target_secondary_opt =
    talkscript_menu_option{2, erdyes::event_text_for_talk::secondary_color, &secondary_state};
static auto dye_target_tertiary_opt =
    talkscript_menu_option{3, erdyes::event_text_for_talk::tertiary_color, &tertiary_state};

static auto dye_target_branch_back_transition =
    from::ezstate::transition{nullptr, from::ezstate::expression{true_expression}};

static auto dye_target_branch_transitions = std::array{
    &dye_target_primary_opt.transition,
    &dye_target_secondary_opt.transition,
    &dye_target_tertiary_opt.transition,
    &dye_target_branch_back_transition,
};
static auto dye_target_branch_state =
    from::ezstate::state{.transitions = from::ezstate::transitions{dye_target_branch_transitions}};

static auto dye_target_events = std::array{
    from::ezstate::event(from::talk_command::close_shop_message),
    from::ezstate::event(from::talk_command::clear_talk_list_data),
    from::ezstate::event(from::talk_command::add_talk_list_data, dye_target_primary_opt.args),
    from::ezstate::event(from::talk_command::add_talk_list_data, dye_target_secondary_opt.args),
    from::ezstate::event(from::talk_command::add_talk_list_data, dye_target_tertiary_opt.args),
    from::ezstate::event(from::talk_command::add_talk_list_data, cancel_args),
    from::ezstate::event(from::talk_command::show_shop_message,
                         show_generic_dialog_shop_message_args),
};
static auto dye_target_transition = from::ezstate::transition{
    &dye_target_branch_state, from::ezstate::expression{talk_menu_closed_expression}};
static auto dye_target_transitions = std::array{&dye_target_transition};
static auto dye_target_state =
    from::ezstate::state{.transitions = dye_target_transitions, .entry_events = dye_target_events};

static auto apply_dyes_option_index = make_int_expression(67);
static auto apply_dyes_message_id = make_int_expression(erdyes::event_text_for_talk::apply_dyes);
static auto apply_dyes_args = std::array{
    from::ezstate::expression(apply_dyes_option_index),
    from::ezstate::expression(apply_dyes_message_id),
    from::ezstate::expression(placeholder_expression),
};
static auto apply_dyes_expression = make_talk_list_result_expression(67);
static auto apply_dyes_transition = from::ezstate::transition{
    &dye_target_state,
    apply_dyes_expression,
};

static void initialize_talkscript_states()
{
    auto &colors = erdyes::state::colors;

    color_opts.clear();
    color_opts.reserve(colors.size());

    color_events.clear();
    color_events.reserve(colors.size() + 5);
    color_events.emplace_back(from::talk_command::close_shop_message);
    color_events.emplace_back(from::talk_command::clear_talk_list_data);
    color_events.emplace_back(from::talk_command::add_talk_list_data, color_none_opt.args);

    color_branch_transitions.clear();
    color_branch_transitions.reserve(colors.size() + 2);
    color_branch_transitions.push_back(&color_none_opt.transition);

    color_selected_states.clear();
    color_selected_states.reserve(colors.size());

    // Create AddTalkListData() events and transitions for each color
    for (int i = 0; i < colors.size(); i++)
    {
        auto &successor_state = color_selected_states.emplace_back(
            0, from::ezstate::transitions{color_selected_return_transitions});

        auto &color_opt = color_opts.emplace_back(
            i + 1, erdyes::event_text_for_talk::dye_name_deselected_start + i, &successor_state);

        color_events.emplace_back(from::talk_command::add_talk_list_data, color_opt.args);

        color_branch_transitions.push_back(&color_opt.transition);
    }

    color_events.emplace_back(from::talk_command::add_talk_list_data, back_args);
    color_events.emplace_back(from::talk_command::show_shop_message,
                              show_generic_dialog_shop_message_args);
    color_state.entry_events = from::ezstate::events{color_events};

    color_branch_transitions.push_back(&color_branch_back_transition);
    color_branch_state.transitions = from::ezstate::transitions{color_branch_transitions};
}

static void update_talkscript_states(int selected_index)
{
    for (int i = 0; i < color_opts.size(); i++)
    {
        auto base_message_id = selected_index == i
                                   ? erdyes::event_text_for_talk::dye_name_selected_start
                                   : erdyes::event_text_for_talk::dye_name_deselected_start;
        color_opts[i].message = make_int_expression(base_message_id + i);
    }

    auto none_message_id = selected_index == -1 ? erdyes::event_text_for_talk::none_selected
                                                : erdyes::event_text_for_talk::none_deselected;
    color_none_opt.message = make_int_expression(none_message_id);
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

/**
 * Triggers custom mod behavior when entering a patched talkscript state
 */
static void handle_state()
{
}

static from::CS::CSMenuManImp **menu_man_addr;

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
            dye_target_branch_back_transition.target_state = machine->state_group->initial_state;
            color_branch_back_transition.target_state = &dye_target_state;
            color_selected_return_transition.target_state = &dye_target_state;
        }
    }

    if (state == &primary_state)
    {
        erdyes::talkscript::dye_target = erdyes::talkscript::dye_target_type::primary_color;
        update_talkscript_states(erdyes::state::primary_color_index);
    }
    else if (state == &secondary_state)
    {
        erdyes::talkscript::dye_target = erdyes::talkscript::dye_target_type::secondary_color;
        update_talkscript_states(erdyes::state::secondary_color_index);
    }
    else if (state == &tertiary_state)
    {
        erdyes::talkscript::dye_target = erdyes::talkscript::dye_target_type::tertiary_color;
        update_talkscript_states(erdyes::state::tertiary_color_index);
    }
    else
    {
        for (int i = 0; i < color_selected_states.size(); i++)
        {
            // Store the selected primary, secondary, or tertiary color when entering that state
            if (state == &color_selected_states[i])
            {
                switch (erdyes::talkscript::dye_target)
                {
                case erdyes::talkscript::dye_target_type::primary_color:
                    erdyes::state::primary_color_index = i;
                    break;
                case erdyes::talkscript::dye_target_type::secondary_color:
                    erdyes::state::secondary_color_index = i;
                    break;
                case erdyes::talkscript::dye_target_type::tertiary_color:
                    erdyes::state::tertiary_color_index = i;
                    break;
                }

                break;
            }
        }

        if (state == &color_none_selected_state)
        {
            switch (erdyes::talkscript::dye_target)
            {
            case erdyes::talkscript::dye_target_type::primary_color:
                erdyes::state::primary_color_index = -1;
                break;
            case erdyes::talkscript::dye_target_type::secondary_color:
                erdyes::state::secondary_color_index = -1;
                break;
            case erdyes::talkscript::dye_target_type::tertiary_color:
                erdyes::state::tertiary_color_index = -1;
                break;
            }
        }

        if (state != &color_state && state != &color_branch_state)
        {
            erdyes::talkscript::dye_target = erdyes::talkscript::dye_target_type::none;
        }
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

    menu_man_addr = modutils::scan<from::CS::CSMenuManImp *>({
        .aob = "48 8b 05 ?? ?? ?? ??" // mov rax, qword ptr [CSMenuMan]
               "33 db"                // xor ebx, ebx
               "48 89 74 ??",         // mov qword ptr [rsp + ??], rsi
        .relative_offsets = {{3, 7}},
    });
}

int erdyes::talkscript::get_focused_entry()
{
    auto menu_man = *menu_man_addr;
    if (menu_man && menu_man->popup_menu && menu_man->popup_menu->window)
    {
        auto dialog =
            reinterpret_cast<from::CS::GenericListSelectDialog *>(menu_man->popup_menu->window);

        if (dialog->grid_control.current_item >= 0 &&
            dialog->grid_control.current_item < dialog->grid_control.item_count)
        {
            return dialog->grid_control.current_item;
        }
    }

    return -1;
}
