/** @file
  lvgl Demo Application - validates GOP rendering and keyboard input (P1 milestone)
**/

#include <Uefi.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PrintLib.h>
#include <Library/LvglLib.h>

#include "lvgl/lvgl.h"

STATIC lv_obj_t   *mLabel   = NULL;
STATIC UINT32      mCounter = 0;

STATIC VOID
TimerCb (
  lv_timer_t  *Timer
  )
{
  CHAR8  Buf[32];

  mCounter++;
  AsciiSPrint (Buf, sizeof (Buf), "Tick: %u", mCounter);
  lv_label_set_text (mLabel, Buf);
}

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS   Status;
  lv_obj_t    *Btn;
  lv_obj_t    *BtnLabel;
  EFI_INPUT_KEY  Key;

  Status = UefiLvglInit (ImageHandle, SystemTable);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  mLabel = lv_label_create (lv_screen_active ());
  lv_label_set_text (mLabel, "Hello UEFI lvgl!");
  lv_obj_align (mLabel, LV_ALIGN_TOP_MID, 0, 20);

  Btn = lv_button_create (lv_screen_active ());
  lv_obj_align (Btn, LV_ALIGN_CENTER, 0, 0);
  BtnLabel = lv_label_create (Btn);
  lv_label_set_text (BtnLabel, "Click Me");

  lv_timer_create (TimerCb, 1000, NULL);

  while (TRUE) {
    UefiLvglTick ();

    if (!EFI_ERROR (gST->ConIn->ReadKeyStroke (gST->ConIn, &Key))) {
      if (Key.ScanCode == SCAN_ESC) {
        break;
      }
    }
  }

  UefiLvglDeinit ();
  return EFI_SUCCESS;
}
