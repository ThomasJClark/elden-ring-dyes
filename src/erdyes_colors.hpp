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

extern std::vector<color> colors;
extern std::vector<intensity> intensities;

/**
 * TEMPORARY in-memory selected color.
 *
 * @todo store this in event flags or items or something
 */
extern int primary_color_index;
extern int secondary_color_index;
extern int tertiary_color_index;

extern int primary_intensity_index;
extern int secondary_intensity_index;
extern int tertiary_intensity_index;

void apply_colors_init();

}
