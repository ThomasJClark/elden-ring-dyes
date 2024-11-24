#pragma once

#include "erdyes_dye_values.hpp"

namespace erdyes
{
namespace net_players
{

// Send messages to connected players containing this player's dye state
void send_messages(const erdyes::state::dye_values &local_player_dyes);

// Check for messages from other players syncing their dye state
void receive_messages();

// Get the dye state sent by another player connected via Seamless Co-op
erdyes::state::dye_values get_selected_dyes(unsigned long long steam_id);

}
}
