#pragma once

#include <string>
#include <vector>

namespace erdyes
{

struct color
{
    std::wstring selected_message;
    std::wstring deselected_message;
    float red;
    float green;
    float blue;
};

struct intensity
{
    std::wstring selected_message;
    std::wstring deselected_message;
    float intensity;
};

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

// Available colors/intensities that can be selected
extern std::vector<color> colors;
extern std::vector<intensity> intensities;

void state_init();

/**
 * Add an option that can be chosen as the primary, secondary, or tertiary dye color
 */
void add_color_option(const std::wstring &name, const std::wstring &hex_code, float r, float g,
                      float b);

/**
 * Add an option that can be chosen as the intensity of the primary, secondary, or tertiary color
 */
void add_intensity_option(const std::wstring &name, const std::wstring &hex_code, float i);

/**
 * @returns the selected index of the given color or intensity option to the given index, or -1 for
 * none
 */
int get_selected_option(erdyes::dye_target_type);

/**
 * Set one of the color or intensity option to the given index, or -1 for none
 */
void set_selected_option(erdyes::dye_target_type, int index);
}
