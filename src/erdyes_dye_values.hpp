#pragma once

namespace erdyes {
namespace state {

struct dye_value {
    bool is_applied{false};
    float red;
    float green;
    float blue;
    float intensity;
};

struct dye_values {
    dye_value primary;
    dye_value secondary;
    dye_value tertiary;
};

}
}
