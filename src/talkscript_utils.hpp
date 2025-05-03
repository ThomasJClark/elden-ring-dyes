#pragma once

#include <elden-x/ezstate/ezstate.hpp>
#include <elden-x/ezstate/talk_commands.hpp>

#include <array>
#include <memory>
#include <vector>

typedef std::array<unsigned char, 6> int_expression;

/**
 * Create an ESD expression representing a 4 byte integer
 */
static constexpr int_expression make_int_expression(int value) {
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
static int get_ezstate_int_value(const er::ezstate::expression arg) {
    // Single byte (plus final 0xa1) - used to store integers from -64 to 63
    if (arg.size() == 2) {
        return arg[0] - 64;
    }

    // Five bytes (plus final 0xa1) - used to store larger integers
    if (arg.size() == 6 && arg[0] == 0x82) {
        return *reinterpret_cast<int *>(&arg[1]);
    }

    return -1;
}

/**
 * Create an ESD expression that checks for GetTalkListEntryResult() returning a given value
 */
static constexpr std::array<unsigned char, 9> make_talk_list_result_expression(int value) {
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
    er::ezstate::expression(generic_dialog_shop_message),
};

/**
 * Common ESD expression that evaluates to true for the final "else" transition
 */
static auto true_expression = std::array<unsigned char, 2>{0x41, 0xa1};

// clang-format off
/**
 * Common ESD expression for the generic talk menu closing
 */
static auto talk_menu_closed_expression = std::array<unsigned char, 40>{
    0x7b,                          // CheckSpecificPersonMenuIsOpen
    0x82, 0x01, 0x00, 0x00, 0x00,  // 1
    0x82, 0x00, 0x00, 0x00, 0x00,  // 0
    0x86,                          // <call with 2 args>
    0x82, 0x01, 0x00, 0x00, 0x00,  // 1
    0x95,                          // ==
    0x7a,                          // CheckSpecificPersonGenericDialogIsOpen
    0x82, 0x00, 0x00, 0x00, 0x00,  // 0
    0x85,                          // <call with 1 arg>
    0x82, 0x00, 0x00, 0x00, 0x00,  // 0
    0x95,                          // ==
    0x98,                          // &&
    0x82, 0x00, 0x00, 0x00, 0x00,  // 0
    0x95,                          // ==
    0xa1,                          // <end>
};
// clang-format on

/**
 * Helper that holds commonly used structs for a talk menu list item
 */
class talkscript_menu_option {
public:
    std::array<unsigned char, 6> index;
    std::array<unsigned char, 6> message;
    std::array<unsigned char, 9> condition;
    std::array<er::ezstate::expression, 3> args;
    er::ezstate::transition transition;
    bool is_default;

    talkscript_menu_option()
        : talkscript_menu_option(-1, -1, nullptr) {}

    talkscript_menu_option(const talkscript_menu_option &other)
        : index(other.index),
          message(other.message),
          condition(other.condition),
          args{this->index, this->message, placeholder_expression},
          transition(other.transition.target_state,
                     other.is_default ? er::ezstate::expression{true_expression}
                                      : er::ezstate::expression{this->condition}),
          is_default(other.is_default) {}

    talkscript_menu_option(int index,
                           int message_id,
                           er::ezstate::state *target_state,
                           bool is_default = false)
        : index(make_int_expression(index)),
          message(make_int_expression(message_id)),
          condition(make_talk_list_result_expression(index)),
          args{this->index, this->message, placeholder_expression},
          transition(target_state,
                     is_default ? er::ezstate::expression{true_expression}
                                : er::ezstate::expression{this->condition}),
          is_default(is_default) {}
};

/**
 * Helper that owns the structs necessary for a typical talk menu list
 */
class talkscript_menu {
public:
    er::ezstate::state state;
    std::vector<talkscript_menu_option> opts;
    std::unique_ptr<er::ezstate::event[]> events;
    er::ezstate::transition transition{&branch_state,
                                       er::ezstate::expression{talk_menu_closed_expression}};
    std::array<er::ezstate::transition *, 1> transitions{&transition};

    er::ezstate::state branch_state;
    std::unique_ptr<er::ezstate::transition *[]> branch_transitions;

    talkscript_menu() { initialize(); }

    talkscript_menu(std::vector<talkscript_menu_option> &&opts_arg)
        : opts(std::move(opts_arg)) {
        initialize();
    }

    void set_opts(std::vector<talkscript_menu_option> &opts_arg) {
        opts = std::move(opts_arg);
        initialize();
    }

private:
    void initialize() {
        size_t branch_transition_count = 0;
        size_t event_count = 0;

        branch_transitions = std::make_unique<er::ezstate::transition *[]>(opts.size());

        events = std::make_unique<er::ezstate::event[]>(opts.size() + 3);
        events[event_count++] = {er::talk_command::close_shop_message};
        events[event_count++] = {er::talk_command::clear_talk_list_data};

        for (auto &opt : opts) {
            events[event_count++] = {er::talk_command::add_talk_list_data, opt.args};
            branch_transitions[branch_transition_count++] = &opt.transition;
        }

        events[event_count++] = {er::talk_command::show_shop_message,
                                 show_generic_dialog_shop_message_args};

        state.entry_events = er::ezstate::events{events.get(), opts.size() + 3};
        state.transitions = transitions;
        branch_state.transitions =
            er::ezstate::transitions{branch_transitions.get(), branch_transition_count};
    }
};
