/** @file
  Two-panel BIOS Setup layout: title bar + left menu + right content + status bar.
**/

#include "GuiDisplayEngine.h"

lv_obj_t  *gRootContainer = NULL;
lv_obj_t  *gTitleBar      = NULL;
lv_obj_t  *gLeftPanel     = NULL;
lv_obj_t  *gRightPanel    = NULL;
lv_obj_t  *gStatusBar     = NULL;

STATIC lv_obj_t    *mTitleLabel = NULL;
STATIC lv_style_t  mTitleBarStyle;
STATIC lv_style_t  mStatusBarStyle;

STATIC VOID
CreateTitleBar (
  IN lv_obj_t  *Parent,
  IN INT32      ScreenW
  )
{
  gTitleBar = lv_obj_create (Parent);
  lv_obj_set_size (gTitleBar, ScreenW, GUI_TITLE_HEIGHT);
  lv_obj_set_pos (gTitleBar, 0, 0);
  lv_obj_clear_flag (gTitleBar, LV_OBJ_FLAG_SCROLLABLE);

  lv_style_init (&mTitleBarStyle);
  lv_style_set_bg_color (&mTitleBarStyle, lv_color_hex (GUI_COLOR_TITLE_BG));
  lv_style_set_border_width (&mTitleBarStyle, 0);
  lv_style_set_radius (&mTitleBarStyle, 0);
  lv_style_set_pad_all (&mTitleBarStyle, 0);
  lv_obj_add_style (gTitleBar, &mTitleBarStyle, 0);

  mTitleLabel = lv_label_create (gTitleBar);
  lv_obj_set_style_text_color (mTitleLabel, lv_color_hex (GUI_COLOR_TITLE_TEXT), 0);
  lv_label_set_text (mTitleLabel, "BIOS Setup");
  lv_obj_align (mTitleLabel, LV_ALIGN_LEFT_MID, 16, 0);
}

STATIC VOID
CreateStatusBar (
  IN lv_obj_t  *Parent,
  IN INT32      ScreenW,
  IN INT32      ScreenH
  )
{
  STATIC CONST CHAR8  *HotKeys[] = {
    "ESC  Back", "F1  Help", "F9  Defaults", "F10  Save & Exit", NULL
  };
  INT32      i;
  INT32      BtnW;
  lv_obj_t  *Btn;
  lv_obj_t  *Label;

  gStatusBar = lv_obj_create (Parent);
  lv_obj_set_size (gStatusBar, ScreenW, GUI_STATUS_HEIGHT);
  lv_obj_set_pos (gStatusBar, 0, ScreenH - GUI_STATUS_HEIGHT);
  lv_obj_clear_flag (gStatusBar, LV_OBJ_FLAG_SCROLLABLE);

  lv_style_init (&mStatusBarStyle);
  lv_style_set_bg_color (&mStatusBarStyle, lv_color_hex (GUI_COLOR_STATUS_BG));
  lv_style_set_border_width (&mStatusBarStyle, 0);
  lv_style_set_radius (&mStatusBarStyle, 0);
  lv_style_set_pad_all (&mStatusBarStyle, 0);
  lv_obj_add_style (gStatusBar, &mStatusBarStyle, 0);

  BtnW = ScreenW / 4;
  for (i = 0; HotKeys[i] != NULL; i++) {
    Btn = lv_button_create (gStatusBar);
    lv_obj_set_size (Btn, BtnW - 4, GUI_STATUS_HEIGHT - 4);
    lv_obj_set_pos (Btn, i * BtnW + 2, 2);
    lv_obj_set_style_bg_color (Btn, lv_color_hex (0x1A5276), 0);
    lv_obj_set_style_border_width (Btn, 0, 0);
    lv_obj_set_style_radius (Btn, 2, 0);

    Label = lv_label_create (Btn);
    lv_label_set_text (Label, HotKeys[i]);
    lv_obj_set_style_text_color (Label, lv_color_hex (GUI_COLOR_STATUS_TEXT), 0);
    lv_obj_align (Label, LV_ALIGN_CENTER, 0, 0);
  }
}

STATIC VOID
CreatePanels (
  IN lv_obj_t  *Parent,
  IN INT32      ScreenW,
  IN INT32      ScreenH
  )
{
  INT32  ContentH;
  INT32  LeftW;
  INT32  RightW;

  ContentH = ScreenH - GUI_TITLE_HEIGHT - GUI_STATUS_HEIGHT;
  LeftW    = ScreenW * GUI_LEFT_PANEL_RATIO / 100;
  RightW   = ScreenW - LeftW;

  gLeftPanel = lv_obj_create (Parent);
  lv_obj_set_size (gLeftPanel, LeftW, ContentH);
  lv_obj_set_pos (gLeftPanel, 0, GUI_TITLE_HEIGHT);
  lv_obj_set_style_bg_color (gLeftPanel, lv_color_hex (0xE8EDF2), 0);
  lv_obj_set_style_border_width (gLeftPanel, 0, 0);
  lv_obj_set_style_radius (gLeftPanel, 0, 0);
  lv_obj_set_style_pad_all (gLeftPanel, 0, 0);

  gRightPanel = lv_obj_create (Parent);
  lv_obj_set_size (gRightPanel, RightW, ContentH);
  lv_obj_set_pos (gRightPanel, LeftW, GUI_TITLE_HEIGHT);
  lv_obj_set_style_bg_color (gRightPanel, lv_color_hex (GUI_COLOR_PANEL), 0);
  lv_obj_set_style_border_width (gRightPanel, 0, 0);
  lv_obj_set_style_radius (gRightPanel, 0, 0);
  lv_obj_set_style_pad_all (gRightPanel, 4, 0);
}

EFI_STATUS
GuiLayoutInit (
  IN INT32  ScreenW,
  IN INT32  ScreenH
  )
{
  gRootContainer = lv_screen_active ();
  lv_obj_set_style_bg_color (gRootContainer, lv_color_hex (GUI_COLOR_BG), 0);
  lv_obj_clear_flag (gRootContainer, LV_OBJ_FLAG_SCROLLABLE);

  CreateTitleBar (gRootContainer, ScreenW);
  CreateStatusBar (gRootContainer, ScreenW, ScreenH);
  CreatePanels (gRootContainer, ScreenW, ScreenH);

  return EFI_SUCCESS;
}

VOID
GuiLayoutSetTitle (
  IN CONST CHAR8  *Title
  )
{
  if (mTitleLabel != NULL) {
    lv_label_set_text (mTitleLabel, Title);
  }
}
