/** @file
  GuiDisplayEngineDxe — installs EDKII_FORM_DISPLAY_ENGINE_PROTOCOL and
  implements FormDisplay / ExitDisplay using lvgl.
**/

#include "GuiDisplayEngine.h"

STATIC EDKII_FORM_DISPLAY_ENGINE_PROTOCOL  mGuiDisplayEngine = {
  GuiFormDisplay,
  GuiExitDisplay,
  NULL
};

STATIC EFI_HANDLE  mDriverHandle = NULL;
STATIC BOOLEAN     mLvglInited   = FALSE;

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

CHAR16 *
GuiGetHiiString (
  IN EFI_HII_HANDLE  HiiHandle,
  IN EFI_STRING_ID   StringId
  )
{
  EFI_STATUS               Status;
  EFI_HII_STRING_PROTOCOL  *HiiString;
  CHAR16                   *Str;
  UINTN                    Len;
  CHAR8                    Lang[] = "en-US";

  Status = gBS->LocateProtocol (
                  &gEfiHiiStringProtocolGuid,
                  NULL,
                  (VOID **)&HiiString
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Len = 256 * sizeof (CHAR16);
  Str = AllocatePool (Len);
  if (Str == NULL) {
    return NULL;
  }

  Status = HiiString->GetString (
                         HiiString,
                         Lang,
                         HiiHandle,
                         StringId,
                         Str,
                         &Len,
                         NULL
                         );
  if (EFI_ERROR (Status)) {
    FreePool (Str);
    return NULL;
  }

  return Str;
}

EFI_STATUS
EFIAPI
GuiFormDisplay (
  IN FORM_DISPLAY_ENGINE_FORM  *FormData,
  OUT USER_INPUT               *UserInput
  )
{
  LIST_ENTRY                    *Link;
  FORM_DISPLAY_ENGINE_STATEMENT *Statement;
  CHAR16                        *TitleStr;
  CHAR8                          TitleUtf8[256];
  EFI_INPUT_KEY                  Key;

  if (!mLvglInited) {
    UefiLvglInit (gImageHandle, gST);
    GuiLayoutInit (LV_HOR_RES, LV_VER_RES);
    mLvglInited = TRUE;
  }

  lv_obj_clean (gRightPanel);
  lv_obj_set_flex_flow (gRightPanel, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align (
    gRightPanel,
    LV_FLEX_ALIGN_START,
    LV_FLEX_ALIGN_START,
    LV_FLEX_ALIGN_START
    );

  TitleStr = GuiGetHiiString (FormData->HiiHandle, FormData->FormTitle);
  if (TitleStr != NULL) {
    Char16ToUtf8 (TitleStr, TitleUtf8, sizeof (TitleUtf8));
    GuiLayoutSetTitle (TitleUtf8);
    FreePool (TitleStr);
  }

  BASE_LIST_FOR_EACH (Link, &FormData->StatementListHead) {
    Statement = BASE_CR (Link, FORM_DISPLAY_ENGINE_STATEMENT, DisplayLink);
    GuiCreateWidget (gRightPanel, Statement, FormData->HiiHandle);
  }

  ZeroMem (UserInput, sizeof (USER_INPUT));

  while (TRUE) {
    UefiLvglTick ();

    if (!EFI_ERROR (gST->ConIn->ReadKeyStroke (gST->ConIn, &Key))) {
      if (Key.ScanCode == SCAN_ESC) {
        UserInput->Action = BROWSER_ACTION_FORM_EXIT;
        break;
      }

      if (Key.ScanCode == SCAN_F10) {
        UserInput->Action = BROWSER_ACTION_SUBMIT;
        break;
      }

      if (Key.ScanCode == SCAN_F9) {
        UserInput->Action = BROWSER_ACTION_DEFAULT;
        break;
      }
    }
  }

  return EFI_SUCCESS;
}

VOID
EFIAPI
GuiExitDisplay (
  VOID
  )
{
  if (mLvglInited) {
    UefiLvglDeinit ();
    mLvglInited = FALSE;
  }
}

EFI_STATUS
EFIAPI
InitializeGuiDisplayEngine (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = gBS->InstallProtocolInterface (
                  &mDriverHandle,
                  &gEdkiiFormDisplayEngineProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mGuiDisplayEngine
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

EFI_STATUS
EFIAPI
UnloadGuiDisplayEngine (
  IN EFI_HANDLE  ImageHandle
  )
{
  GuiExitDisplay ();
  gBS->UninstallProtocolInterface (
         mDriverHandle,
         &gEdkiiFormDisplayEngineProtocolGuid,
         &mGuiDisplayEngine
         );
  return EFI_SUCCESS;
}
