#pragma once

#include "erdyes_local_player.hpp"

namespace erdyes {

void setup_talkscript();

/** Returns index of the currently selected menu option, or -1 if none */
int get_talkscript_focused_entry();

/** Returns the dye option currently being edited in the talkscript menu */
erdyes::dye_target_type get_talkscript_dye_target();

}
