/**
 * erdyes_net_players.cpp
 *
 * Sends and receives messages to other players in Seamless Co-op so dye state can be synced between
 * multiple players with this mod installed
 */
#include "erdyes_net_players.hpp"

#include <spdlog/spdlog.h>
#include <steam/isteamnetworkingmessages.h>
#include <steam/isteamuser.h>

#include <elden-x/session.hpp>

#include <algorithm>
#include <map>
#include <span>

using namespace std;

static map<unsigned long long, erdyes::state::dye_values> net_colors_map;

static constexpr int steam_networking_channel_dyes = 100067;

void erdyes::net_players::send_messages(const erdyes::state::dye_values &local_player_dyes) {
    auto session_manager = er::CS::CSSessionManagerImp::instance();

    auto local_player_steam_id = SteamUser()->GetSteamID().ConvertToUint64();

    // Send the local player's dye selections to every connected player in the current session
    for (auto &entry : session_manager->player_entries()) {
        // Don't send messages to ourself
        if (entry.steam_id == local_player_steam_id) {
            continue;
        }

        SteamNetworkingIdentity id;
        id.SetSteamID(entry.steam_id);

        auto result = SteamNetworkingMessages()->SendMessageToUser(
            id, &local_player_dyes, sizeof(local_player_dyes), k_nSteamNetworkingSend_Reliable,
            steam_networking_channel_dyes);
        if (result != k_EResultOK) {
            spdlog::error("Error {} sending Steam networking message to user {}", (int)result,
                          id.GetSteamID64());
        }
    }
}

void erdyes::net_players::receive_messages() {
    static SteamNetworkingMessage_t *buffer[100];

    // Check for new messages from the Steamworks API containing the dye state of other connected
    // players
    auto count = SteamNetworkingMessages()->ReceiveMessagesOnChannel(
        steam_networking_channel_dyes, buffer, sizeof(buffer) / sizeof(buffer[0]));
    auto messages = span{buffer, static_cast<size_t>(count)};

    for (auto &message : messages) {
        auto steam_id = message->m_identityPeer.GetSteamID64();
        auto &net_colors = *reinterpret_cast<const erdyes::state::dye_values *>(message->GetData());

        if (!net_colors_map.contains(steam_id)) {
            spdlog::debug("Received dye values from user {}", steam_id);
        }

        net_colors_map[steam_id] = net_colors;
        message->Release();
    }

    // Remove entries for players who aren't connected anymore
    auto player_entries = er::CS::CSSessionManagerImp::instance()->player_entries();
    for (auto it = net_colors_map.begin(); it != net_colors_map.end();) {
        if (none_of(player_entries.begin(), player_entries.end(),
                    [&](auto entry) { return entry.steam_id == it->first; })) {
            spdlog::debug("Disconnected from user {}", it->first);
            it = net_colors_map.erase(it);
        } else {
            it++;
        }
    }
}

erdyes::state::dye_values erdyes::net_players::get_selected_dyes(unsigned long long steam_id) {
    auto entry = net_colors_map.find(steam_id);
    if (entry != net_colors_map.end()) {
        return entry->second;
    }

    return erdyes::state::dye_values{};
}
