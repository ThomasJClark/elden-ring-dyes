#pragma once

#include "chr.hpp"

namespace from
{
namespace CS
{

struct net_player
{
    CS::PlayerIns *player;
    unsigned char unk[0x8];
};

struct WorldChrManImp
{
    unsigned char unk1[0x10EF8];
    net_player *net_players;
    unsigned char unk2[0xd608];
    CS::PlayerIns *main_player;
};

}
}
