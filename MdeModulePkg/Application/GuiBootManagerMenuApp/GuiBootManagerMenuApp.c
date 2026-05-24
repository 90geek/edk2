/** @file
  GuiBootManagerMenuApp — lvgl-based graphical boot device selection menu.
  Drop-in replacement for BootManagerMenuApp.
**/

#include <Uefi.h>
#include <Protocol/GraphicsOutput.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/LvglLib.h>
#include "lvgl/lvgl.h"

STATIC VOID
Char16ToUtf8 (
  IN  CONST CHAR16  *Src,
  OUT CHAR8         *Dst,
  IN  UINTN          DstSize
  )
{
  UINTN   i;
  UINTN   j;
  UINT16  c;

  for (i = 0, j = 0; Src[i] != 0 && j + 4 < DstSize; i++) {
    c = (UINT16)Src[i];
    if (c < 0x80) {
      Dst[j++] = (CHAR8)c;
    } else if (c < 0x800) {
      Dst[j++] = (CHAR8)(0xC0 | (c >> 6));
      Dst[j++] = (CHAR8)(0x80 | (c & 0x3F));
    } else {
      Dst[j++] = (CHAR8)(0xE0 | (c >> 12));
      Dst[j++] = (CHAR8)(0x80 | ((c >> 6) & 0x3F));
      Dst[j++] = (CHAR8)(0x80 | (c & 0x3F));
    }
  }

  Dst[j] = '\0';
}

#define MAX_BOOT_OPTIONS  64
#define ITEM_HEIGHT       48

STATIC lv_obj_t  *gItems[MAX_BOOT_OPTIONS];
STATIC UINTN      gSelectedIdx  = 0;
STATIC UINTN      gOptionCount  = 0;

STATIC VOID
UpdateSelection (
  IN UINTN  NewIdx
  )
{
  if (gOptionCount == 0) {
    return;
  }

  lv_obj_set_style_bg_color (gItems[gSelectedIdx], lv_color_hex (0x2A2A3E), LV_PART_MAIN);

  gSelectedIdx = NewIdx;

  lv_obj_set_style_bg_color (gItems[gSelectedIdx], lv_color_hex (0x0078D4), LV_PART_MAIN);
  lv_obj_scroll_to_view (gItems[gSelectedIdx], LV_ANIM_OFF);
}

EFI_STATUS
EFIAPI
BootManagerMenuEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOptions;
  UINTN                          Count;
  lv_obj_t                      *Screen;
  lv_obj_t                      *TitleBar;
  lv_obj_t                      *TitleLabel;
  lv_obj_t                      *List;
  lv_obj_t                      *StatusBar;
  lv_obj_t                      *StatusLabel;
  lv_obj_t                      *Item;
  lv_obj_t                      *ItemLabel;
  CHAR8                          Buf[256];
  UINTN                          i;
  EFI_INPUT_KEY                  Key;
  BOOLEAN                        Done;

  UefiLvglInit (ImageHandle, SystemTable);

  Screen = lv_screen_active ();
  lv_obj_set_style_bg_color (Screen, lv_color_hex (0x1A1A2E), LV_PART_MAIN);

  TitleBar = lv_obj_create (Screen);
  lv_obj_set_size (TitleBar, lv_pct (100), 48);
  lv_obj_align (TitleBar, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_bg_color (TitleBar, lv_color_hex (0x2B5797), LV_PART_MAIN);
  lv_obj_set_style_border_width (TitleBar, 0, LV_PART_MAIN);
  lv_obj_set_style_radius (TitleBar, 0, LV_PART_MAIN);

  TitleLabel = lv_label_create (TitleBar);
  lv_label_set_text (TitleLabel, "Boot Device Selection");
  lv_obj_align (TitleLabel, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_color (TitleLabel, lv_color_hex (0xFFFFFF), LV_PART_MAIN);

  List = lv_obj_create (Screen);
  lv_obj_set_pos (List, 0, 48);
  lv_obj_set_size (List, lv_pct (100), LV_VER_RES - 48 - 40);
  lv_obj_set_style_bg_color (List, lv_color_hex (0x1A1A2E), LV_PART_MAIN);
  lv_obj_set_style_border_width (List, 0, LV_PART_MAIN);
  lv_obj_set_style_radius (List, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all (List, 8, LV_PART_MAIN);
  lv_obj_set_style_pad_row (List, 4, LV_PART_MAIN);
  lv_obj_set_flex_flow (List, LV_FLEX_FLOW_COLUMN);

  StatusBar = lv_obj_create (Screen);
  lv_obj_set_size (StatusBar, lv_pct (100), 40);
  lv_obj_align (StatusBar, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_bg_color (StatusBar, lv_color_hex (0x1E3A6E), LV_PART_MAIN);
  lv_obj_set_style_border_width (StatusBar, 0, LV_PART_MAIN);
  lv_obj_set_style_radius (StatusBar, 0, LV_PART_MAIN);

  StatusLabel = lv_label_create (StatusBar);
  lv_label_set_text (StatusLabel, "Up/Down: Navigate    Enter: Boot    Esc: Exit");
  lv_obj_align (StatusLabel, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_color (StatusLabel, lv_color_hex (0xAAAAAA), LV_PART_MAIN);

  BootOptions  = EfiBootManagerGetLoadOptions (&Count, LoadOptionTypeBoot);
  gOptionCount = (Count < MAX_BOOT_OPTIONS) ? Count : MAX_BOOT_OPTIONS;

  for (i = 0; i < gOptionCount; i++) {
    Item = lv_obj_create (List);
    lv_obj_set_size (Item, lv_pct (100), ITEM_HEIGHT);
    lv_obj_set_style_bg_color (Item, lv_color_hex (0x2A2A3E), LV_PART_MAIN);
    lv_obj_set_style_border_width (Item, 0, LV_PART_MAIN);
    lv_obj_set_style_radius (Item, 6, LV_PART_MAIN);

    ItemLabel = lv_label_create (Item);
    if (BootOptions[i].Description != NULL) {
      Char16ToUtf8 (BootOptions[i].Description, Buf, sizeof (Buf));
    } else {
      AsciiSPrint (Buf, sizeof (Buf), "Boot%04X", (UINT32)BootOptions[i].OptionNumber);
    }

    lv_label_set_text (ItemLabel, Buf);
    lv_obj_align (ItemLabel, LV_ALIGN_LEFT_MID, 12, 0);
    lv_obj_set_style_text_color (ItemLabel, lv_color_hex (0xFFFFFF), LV_PART_MAIN);

    gItems[i] = Item;
  }

  if (gOptionCount > 0) {
    UpdateSelection (0);
  }

  UefiLvglTick ();

  Done = FALSE;
  while (!Done) {
    UefiLvglTick ();

    if (!EFI_ERROR (gST->ConIn->ReadKeyStroke (gST->ConIn, &Key))) {
      if (Key.ScanCode == SCAN_UP) {
        if (gSelectedIdx > 0) {
          UpdateSelection (gSelectedIdx - 1);
        }
      } else if (Key.ScanCode == SCAN_DOWN) {
        if (gSelectedIdx + 1 < gOptionCount) {
          UpdateSelection (gSelectedIdx + 1);
        }
      } else if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
        if (gOptionCount > 0) {
          EfiBootManagerBoot (&BootOptions[gSelectedIdx]);
        }
      } else if (Key.ScanCode == SCAN_ESC) {
        Done = TRUE;
      }

      UefiLvglTick ();
    }
  }

  EfiBootManagerFreeLoadOptions (BootOptions, Count);
  UefiLvglDeinit ();
  return EFI_SUCCESS;
}
