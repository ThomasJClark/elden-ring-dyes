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
    int entry_count;

    /** The currently selected menu item */
    int focused_entry;
};

struct dialog
{
};

/** State of the current AddTalkListData() dialog */
class CSEventListDialog : public dialog
{
  public:
    virtual ~CSEventListDialog() = default;

    unsigned char unk8[0x48];

    GridControl *grid_control;
};

class CSPopupMenu
{
  public:
    virtual ~CSPopupMenu() = default;

    CSMenuManImp *owner;
    dialog *active_dialog;
};

class CSMenuManImp
{
  public:
    virtual ~CSMenuManImp() = default;

    unsigned char unk8[0x78];

    CSPopupMenu *popup_menu;
};

}
}
