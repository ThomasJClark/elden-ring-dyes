#pragma once

namespace erdyes
{

void setup_talkscript();

namespace talkscript
{

enum class dye_target_type
{
    none,
    primary_color,
    secondary_color,
    tertiary_color,
    primary_intensity,
    secondary_intensity,
    tertiary_intensity,
};

/** Stores the current color option being edited */
extern erdyes::talkscript::dye_target_type dye_target;

/** Returns index of the currently selected menu option, or -1 if none */
int get_focused_entry();

}

}
