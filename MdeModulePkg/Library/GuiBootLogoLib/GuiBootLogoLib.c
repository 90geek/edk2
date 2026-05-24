/** @file
  GuiBootLogoLib — lvgl-based boot logo and progress bar.
  Drop-in replacement for BootLogoLib with graphical rendering.
**/

#include <Uefi.h>
#include <Protocol/GraphicsOutput.h>
#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/LvglLib.h>
#include "lvgl/lvgl.h"

STATIC BOOLEAN    mLvglInited = FALSE;
STATIC lv_obj_t  *mBar       = NULL;
STATIC lv_obj_t  *mLabel     = NULL;

STATIC VOID
EnsureInited (
  VOID
  )
{
  lv_obj_t  *Screen;
  lv_obj_t  *VendorLabel;

  if (mLvglInited) {
    return;
  }

  UefiLvglInit (gImageHandle, gST);

  Screen = lv_screen_active ();
  lv_obj_set_style_bg_color (Screen, lv_color_hex (0x1A1A2E), LV_PART_MAIN);

  VendorLabel = lv_label_create (Screen);
  lv_label_set_text (VendorLabel, "Loongson Technology");
  lv_obj_align (VendorLabel, LV_ALIGN_CENTER, 0, -60);
  lv_obj_set_style_text_color (VendorLabel, lv_color_hex (0xFFFFFF), LV_PART_MAIN);

  mLabel = lv_label_create (Screen);
  lv_label_set_text (mLabel, "Starting...");
  lv_obj_align (mLabel, LV_ALIGN_BOTTOM_MID, 0, -60);
  lv_obj_set_style_text_color (mLabel, lv_color_hex (0xAAAAAA), LV_PART_MAIN);

  mBar = lv_bar_create (Screen);
  lv_obj_set_size (mBar, lv_pct (60), 8);
  lv_obj_align (mBar, LV_ALIGN_BOTTOM_MID, 0, -40);
  lv_bar_set_range (mBar, 0, 100);
  lv_bar_set_value (mBar, 0, LV_ANIM_OFF);
  lv_obj_set_style_bg_color (mBar, lv_color_hex (0x333355), LV_PART_MAIN);
  lv_obj_set_style_bg_color (mBar, lv_color_hex (0x0078D4), LV_PART_INDICATOR);
  lv_obj_set_style_radius (mBar, 4, LV_PART_MAIN);
  lv_obj_set_style_radius (mBar, 4, LV_PART_INDICATOR);

  UefiLvglTick ();
  mLvglInited = TRUE;
}

EFI_STATUS
EFIAPI
BootLogoEnableLogo (
  VOID
  )
{
  EnsureInited ();
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
BootLogoDisableLogo (
  VOID
  )
{
  if (mLvglInited) {
    UefiLvglDeinit ();
    mLvglInited = FALSE;
    mBar        = NULL;
    mLabel      = NULL;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
BootLogoUpdateProgress (
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  TitleForeground,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  TitleBackground,
  IN CHAR16                         *Title,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  ProgressColor,
  IN UINTN                          Progress,
  IN UINTN                          PreviousValue
  )
{
  CHAR8  Buf[64];

  if (Progress > 100) {
    return EFI_INVALID_PARAMETER;
  }

  EnsureInited ();

  if (mBar != NULL) {
    lv_bar_set_value (mBar, (INT32)Progress, LV_ANIM_OFF);
  }

  if (mLabel != NULL && Title != NULL) {
    UINTN  i = 0, j = 0;
    while (Title[i] != 0 && j + 4 < sizeof (Buf)) {
      UINT16  c = (UINT16)Title[i++];
      if (c < 0x80) {
        Buf[j++] = (CHAR8)c;
      } else if (c < 0x800) {
        Buf[j++] = (CHAR8)(0xC0 | (c >> 6));
        Buf[j++] = (CHAR8)(0x80 | (c & 0x3F));
      } else {
        Buf[j++] = (CHAR8)(0xE0 | (c >> 12));
        Buf[j++] = (CHAR8)(0x80 | ((c >> 6) & 0x3F));
        Buf[j++] = (CHAR8)(0x80 | (c & 0x3F));
      }
    }

    Buf[j] = '\0';
    lv_label_set_text (mLabel, Buf);
  }

  UefiLvglTick ();
  return EFI_SUCCESS;
}
