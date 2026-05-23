/** @file
  HII IFR OpCode to lvgl widget factory.
  Each CreateXxx() takes a FORM_DISPLAY_ENGINE_STATEMENT and returns an lv_obj_t*.
**/

#include "GuiDisplayEngine.h"
#include <Uefi/UefiInternalFormRepresentation.h>

STATIC VOID
Char16ToUtf8Simple (
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

STATIC VOID
SetLabelFromHiiString (
  IN lv_obj_t       *Label,
  IN EFI_HII_HANDLE  HiiHandle,
  IN EFI_STRING_ID   StringId
  )
{
  CHAR16  *Str16;
  CHAR8    Buf[512];

  Str16 = GuiGetHiiString (HiiHandle, StringId);
  if (Str16 != NULL) {
    Char16ToUtf8Simple (Str16, Buf, sizeof (Buf));
    lv_label_set_text (Label, Buf);
    FreePool (Str16);
  } else {
    lv_label_set_text (Label, "?");
  }
}

STATIC lv_obj_t *
CreateCheckbox (
  IN lv_obj_t                       *Parent,
  IN FORM_DISPLAY_ENGINE_STATEMENT  *Statement,
  IN EFI_HII_HANDLE                  HiiHandle
  )
{
  lv_obj_t         *Row;
  lv_obj_t         *Label;
  lv_obj_t         *Sw;
  EFI_IFR_CHECKBOX *Cb;

  Cb = (EFI_IFR_CHECKBOX *)Statement->OpCode;

  Row = lv_obj_create (Parent);
  lv_obj_set_size (Row, lv_pct (100), 40);
  lv_obj_set_style_border_width (Row, 0, 0);
  lv_obj_set_style_bg_opa (Row, LV_OPA_TRANSP, 0);
  lv_obj_set_flex_flow (Row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align (Row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  Label = lv_label_create (Row);
  SetLabelFromHiiString (Label, HiiHandle, Cb->Question.Header.Prompt);

  Sw = lv_switch_create (Row);
  if (Statement->CurrentValue.Value.b) {
    lv_obj_add_state (Sw, LV_STATE_CHECKED);
  }

  return Row;
}

STATIC lv_obj_t *
CreateNumeric (
  IN lv_obj_t                       *Parent,
  IN FORM_DISPLAY_ENGINE_STATEMENT  *Statement,
  IN EFI_HII_HANDLE                  HiiHandle
  )
{
  lv_obj_t        *Row;
  lv_obj_t        *Sb;
  EFI_IFR_NUMERIC *Numeric;

  Numeric = (EFI_IFR_NUMERIC *)Statement->OpCode;

  Row = lv_obj_create (Parent);
  lv_obj_set_size (Row, lv_pct (100), 40);
  lv_obj_set_style_border_width (Row, 0, 0);
  lv_obj_set_style_bg_opa (Row, LV_OPA_TRANSP, 0);

  Sb = lv_spinbox_create (Row);
  lv_obj_align (Sb, LV_ALIGN_RIGHT_MID, -8, 0);

  switch (Numeric->Flags & EFI_IFR_NUMERIC_SIZE) {
    case EFI_IFR_NUMERIC_SIZE_1:
      lv_spinbox_set_range (Sb, Numeric->data.u8.MinValue, Numeric->data.u8.MaxValue);
      lv_spinbox_set_value (Sb, Statement->CurrentValue.Value.u8);
      lv_spinbox_set_step (Sb, Numeric->data.u8.Step);
      break;
    case EFI_IFR_NUMERIC_SIZE_2:
      lv_spinbox_set_range (Sb, Numeric->data.u16.MinValue, Numeric->data.u16.MaxValue);
      lv_spinbox_set_value (Sb, Statement->CurrentValue.Value.u16);
      lv_spinbox_set_step (Sb, Numeric->data.u16.Step);
      break;
    case EFI_IFR_NUMERIC_SIZE_4:
      lv_spinbox_set_range (Sb, Numeric->data.u32.MinValue, Numeric->data.u32.MaxValue);
      lv_spinbox_set_value (Sb, Statement->CurrentValue.Value.u32);
      lv_spinbox_set_step (Sb, Numeric->data.u32.Step);
      break;
    default:
      break;
  }

  return Row;
}

STATIC lv_obj_t *
CreateOneOf (
  IN lv_obj_t                       *Parent,
  IN FORM_DISPLAY_ENGINE_STATEMENT  *Statement,
  IN EFI_HII_HANDLE                  HiiHandle
  )
{
  lv_obj_t                *Row;
  lv_obj_t                *Dd;
  LIST_ENTRY              *Link;
  DISPLAY_QUESTION_OPTION *Option;
  CHAR16                  *Str16;
  CHAR8                    Buf[256];
  UINT32                   Idx;
  UINT32                   SelIdx;

  Row = lv_obj_create (Parent);
  lv_obj_set_size (Row, lv_pct (100), 40);
  lv_obj_set_style_border_width (Row, 0, 0);
  lv_obj_set_style_bg_opa (Row, LV_OPA_TRANSP, 0);

  Dd = lv_dropdown_create (Row);
  lv_dropdown_clear_options (Dd);
  lv_obj_align (Dd, LV_ALIGN_RIGHT_MID, -8, 0);
  lv_obj_set_width (Dd, 200);

  Idx    = 0;
  SelIdx = 0;
  BASE_LIST_FOR_EACH (Link, &Statement->OptionListHead) {
    Option = DISPLAY_QUESTION_OPTION_FROM_LINK (Link);
    Str16  = GuiGetHiiString (HiiHandle, Option->OptionOpCode->Option);
    if (Str16 != NULL) {
      Char16ToUtf8Simple (Str16, Buf, sizeof (Buf));
      FreePool (Str16);
    } else {
      AsciiSPrint (Buf, sizeof (Buf), "Option %u", Idx);
    }

    lv_dropdown_add_option (Dd, Buf, Idx);

    if (CompareMem (
          &Option->OptionOpCode->Value,
          &Statement->CurrentValue.Value,
          sizeof (EFI_IFR_TYPE_VALUE)) == 0)
    {
      SelIdx = Idx;
    }

    Idx++;
  }

  lv_dropdown_set_selected (Dd, SelIdx);
  return Row;
}

STATIC lv_obj_t *
CreateDate (
  IN lv_obj_t                       *Parent,
  IN FORM_DISPLAY_ENGINE_STATEMENT  *Statement,
  IN EFI_HII_HANDLE                  HiiHandle
  )
{
  lv_obj_t  *Row;
  lv_obj_t  *RollerY;
  lv_obj_t  *RollerM;
  lv_obj_t  *RollerD;
  CHAR8      YearOpts[640];
  CHAR8      MonthOpts[64];
  CHAR8      DayOpts[128];
  CHAR8      Tmp[8];
  UINT16     y;
  UINT8      d;

  Row = lv_obj_create (Parent);
  lv_obj_set_size (Row, lv_pct (100), 80);
  lv_obj_set_style_border_width (Row, 0, 0);
  lv_obj_set_style_bg_opa (Row, LV_OPA_TRANSP, 0);
  lv_obj_set_flex_flow (Row, LV_FLEX_FLOW_ROW);

  YearOpts[0] = '\0';
  for (y = 2000; y <= 2099; y++) {
    AsciiSPrint (Tmp, sizeof (Tmp), y > 2000 ? "\n%u" : "%u", (UINT32)y);
    AsciiStrCatS (YearOpts, sizeof (YearOpts), Tmp);
  }

  AsciiStrCpyS (MonthOpts, sizeof (MonthOpts),
                "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12");

  DayOpts[0] = '\0';
  for (d = 1; d <= 31; d++) {
    AsciiSPrint (Tmp, sizeof (Tmp), d > 1 ? "\n%u" : "%u", (UINT32)d);
    AsciiStrCatS (DayOpts, sizeof (DayOpts), Tmp);
  }

  RollerY = lv_roller_create (Row);
  lv_roller_set_options (RollerY, YearOpts, LV_ROLLER_MODE_NORMAL);
  lv_roller_set_selected (
    RollerY,
    Statement->CurrentValue.Value.date.Year - 2000,
    LV_ANIM_OFF
    );
  lv_roller_set_visible_row_count (RollerY, 3);

  RollerM = lv_roller_create (Row);
  lv_roller_set_options (RollerM, MonthOpts, LV_ROLLER_MODE_NORMAL);
  lv_roller_set_selected (
    RollerM,
    Statement->CurrentValue.Value.date.Month - 1,
    LV_ANIM_OFF
    );
  lv_roller_set_visible_row_count (RollerM, 3);

  RollerD = lv_roller_create (Row);
  lv_roller_set_options (RollerD, DayOpts, LV_ROLLER_MODE_NORMAL);
  lv_roller_set_selected (
    RollerD,
    Statement->CurrentValue.Value.date.Day - 1,
    LV_ANIM_OFF
    );
  lv_roller_set_visible_row_count (RollerD, 3);

  return Row;
}

lv_obj_t *
GuiCreateWidget (
  IN lv_obj_t                       *Parent,
  IN FORM_DISPLAY_ENGINE_STATEMENT  *Statement,
  IN EFI_HII_HANDLE                  HiiHandle
  )
{
  lv_obj_t  *Btn;

  switch (Statement->OpCode->OpCode) {
    case EFI_IFR_CHECKBOX_OP:
      return CreateCheckbox (Parent, Statement, HiiHandle);

    case EFI_IFR_NUMERIC_OP:
      return CreateNumeric (Parent, Statement, HiiHandle);

    case EFI_IFR_ONE_OF_OP:
      return CreateOneOf (Parent, Statement, HiiHandle);

    case EFI_IFR_DATE_OP:
      return CreateDate (Parent, Statement, HiiHandle);

    case EFI_IFR_STRING_OP:
    case EFI_IFR_PASSWORD_OP:
    case EFI_IFR_ORDERED_LIST_OP:
      return NULL;

    case EFI_IFR_REF_OP:
      Btn = lv_button_create (Parent);
      lv_obj_set_size (Btn, lv_pct (100), 36);
      lv_obj_set_style_bg_color (Btn, lv_color_hex (GUI_COLOR_HIGHLIGHT), 0);
      return Btn;

    default:
      return NULL;
  }
}
