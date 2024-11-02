#pragma once

namespace from
{
namespace CS
{

class CSMenuManImp;

class GridControl
{
  public:
    virtual ~GridControl() = default;

    unsigned char unk8[0xc8];

    /** The number of menu items added to the current dialog */
    int item_count;

    /** The currently selected menu item */
    int current_item;
};

class MenuWindow
{
  public:
    virtual ~MenuWindow() = default;

    unsigned char unk8[0xa30];
};

class GenericListSelectDialog : public CS::MenuWindow
{
  public:
    virtual ~GenericListSelectDialog() = default;

    CS::GridControl grid_control;
};

class CSPopupMenu
{
  public:
    virtual ~CSPopupMenu() = default;

    CS::CSMenuManImp *owner;
    CS::MenuWindow *window;
};

class CSMenuManImp
{
  public:
    virtual ~CSMenuManImp() = default;

    unsigned char unk8[0x78];

    CS::CSPopupMenu *popup_menu;
};

}
}
