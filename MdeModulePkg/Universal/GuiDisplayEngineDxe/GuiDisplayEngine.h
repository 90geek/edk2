/** @file
  GuiDisplayEngineDxe internal types and declarations.
**/

#ifndef GUI_DISPLAY_ENGINE_H_
#define GUI_DISPLAY_ENGINE_H_

#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/LvglLib.h>
#include <Protocol/DisplayProtocol.h>
#include <Protocol/FormBrowserEx.h>
#include <Protocol/HiiString.h>

#include "lvgl/lvgl.h"

#define GUI_TITLE_HEIGHT      32
#define GUI_STATUS_HEIGHT     28
#define GUI_LEFT_PANEL_RATIO  35

#define GUI_COLOR_BG          0xF0F0F0
#define GUI_COLOR_PANEL       0xFFFFFF
#define GUI_COLOR_TITLE_BG    0x2B5797
#define GUI_COLOR_TITLE_TEXT  0xFFFFFF
#define GUI_COLOR_HIGHLIGHT   0x0078D4
#define GUI_COLOR_STATUS_BG   0x1E3A6E
#define GUI_COLOR_STATUS_TEXT 0xFFFFFF

extern lv_obj_t  *gRootContainer;
extern lv_obj_t  *gTitleBar;
extern lv_obj_t  *gLeftPanel;
extern lv_obj_t  *gRightPanel;
extern lv_obj_t  *gStatusBar;

CHAR16 *
GuiGetHiiString (
  IN EFI_HII_HANDLE  HiiHandle,
  IN EFI_STRING_ID   StringId
  );

VOID
GuiLayoutSetTitle (
  IN CONST CHAR8  *Title
  );

EFI_STATUS
GuiLayoutInit (
  IN INT32  ScreenW,
  IN INT32  ScreenH
  );

lv_obj_t *
GuiCreateWidget (
  IN lv_obj_t                       *Parent,
  IN FORM_DISPLAY_ENGINE_STATEMENT  *Statement,
  IN EFI_HII_HANDLE                  HiiHandle
  );

EFI_STATUS
EFIAPI
GuiFormDisplay (
  IN FORM_DISPLAY_ENGINE_FORM  *FormData,
  OUT USER_INPUT               *UserInput
  );

VOID
EFIAPI
GuiExitDisplay (
  VOID
  );

#endif /* GUI_DISPLAY_ENGINE_H_ */
