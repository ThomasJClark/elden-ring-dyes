#pragma once

#include "stl.hpp"

namespace from
{

namespace CS
{

class ChrIns;

class SpecialEffect;

class CSChrModelParamModifierModule
{
    virtual ~CSChrModelParamModifierModule() = default;

  public:
    struct modifier_value
    {
        int material_id;
        float value1; // Red
        float value2; // Green
        float value3; // Blue
        float value4; // Alpha
        float value5; // Intensity
    };
    static_assert(sizeof(modifier_value) == 0x18);

    struct modifier
    {
        bool unk0;
        unsigned int unk4;
        int unk8;
        int unkc;
        unsigned long long unk10;
        unsigned int unk18;
        unsigned int unk1c;
        const wchar_t *name;
        modifier_value unk28;
        modifier_value value;
        modifier_value unk58;
        unsigned int unk70;
        unsigned int unk74;
        unsigned int unk78;
        unsigned int unk7c;
        unsigned int unk80;
        unsigned int unk84;
        modifier_value unk88;
        modifier_value unka0;
        unsigned int unkb0;
        unsigned int unkb4;
        float unkb8;
        unsigned int unkbc;
        unsigned int unkc0;
        float unkc4;
        unsigned int unkc8;
        unsigned int unkcc;
    };
    static_assert(sizeof(modifier) == 0xd8);

    CS::ChrIns *owner;

    from::vector<modifier> modifiers;
};

class ChrIns
{
  public:
    virtual ~ChrIns() = default;

    struct modules_type
    {
        void *unk_modules[26];
        CS::CSChrModelParamModifierModule *model_param_modifier_module;
    };

    unsigned char unk8[0x170];
    CS::SpecialEffect *special_effects;
    unsigned char unk180[0x10];
    modules_type *modules;
};

class PlayerIns : public CS::ChrIns
{
  public:
    virtual ~PlayerIns() = default;
};

}
}
