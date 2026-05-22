/** @file
  lvgl keyboard input adapter for UEFI/EDK2.

  Maps EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL keystrokes to lvgl v9.2.2
  keypad input device events.  The adapter is polled every time lvgl
  calls the read callback from its internal timer.

  Key-mapping strategy
  --------------------
  EFI keystroke data carries two independent fields:
    Key.ScanCode   - non-zero for special/control keys (arrows, F-keys, …)
    Key.UnicodeChar - non-zero for printable and a few control characters

  Priority: ScanCode is checked first.  If ScanCode == SCAN_NULL the
  UnicodeChar is forwarded directly to lvgl (printable text input).

  Copyright (c) 2024, EDK2 Contributors.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "LvglPort.h"

// ---------------------------------------------------------------------------
// Module-level globals
// ---------------------------------------------------------------------------

/** Handle to the located SimpleTextInputEx protocol.  NULL if unavailable. **/
STATIC EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *mKb = NULL;

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

/**
  Map an EFI keystroke to an lvgl key value.

  @param[in]  KeyData   Pointer to the EFI_KEY_DATA read from the protocol.
  @param[out] LvKey     Receives the mapped lvgl key constant (uint32_t).

  @retval TRUE   A mapping was found; *LvKey is valid.
  @retval FALSE  The keystroke should be ignored (e.g. modifier-only event).
**/
STATIC
BOOLEAN
MapEfiKeyToLvgl (
  IN  CONST EFI_KEY_DATA  *KeyData,
  OUT UINT32              *LvKey
  )
{
  UINT16  ScanCode;
  CHAR16  UnicodeChar;

  ScanCode    = KeyData->Key.ScanCode;
  UnicodeChar = KeyData->Key.UnicodeChar;

  //
  // 1. Map EFI scan codes (special / non-printable keys)
  //
  switch (ScanCode) {
    case SCAN_UP:         *LvKey = LV_KEY_UP;        return TRUE;
    case SCAN_DOWN:       *LvKey = LV_KEY_DOWN;       return TRUE;
    case SCAN_RIGHT:      *LvKey = LV_KEY_RIGHT;      return TRUE;
    case SCAN_LEFT:       *LvKey = LV_KEY_LEFT;       return TRUE;
    case SCAN_HOME:       *LvKey = LV_KEY_HOME;       return TRUE;
    case SCAN_END:        *LvKey = LV_KEY_END;        return TRUE;
    case SCAN_DELETE:     *LvKey = LV_KEY_DEL;        return TRUE;
    case SCAN_ESC:        *LvKey = LV_KEY_ESC;        return TRUE;
    case SCAN_PAGE_UP:    *LvKey = LV_KEY_PREV;       return TRUE;
    case SCAN_PAGE_DOWN:  *LvKey = LV_KEY_NEXT;       return TRUE;
    //
    // Function keys — F9/F10 are the common UEFI "Load defaults" / "Save and
    // Exit" convention.  Map them to ESC / ENTER so that lvgl group
    // navigation has a reasonable fallback.  F1 is also mapped to ESC as a
    // safe no-op (LV_KEY_HELP does not exist in v9.2.2).
    //
    case SCAN_F1:   *LvKey = LV_KEY_ESC;    return TRUE;
    case SCAN_F9:   *LvKey = LV_KEY_ESC;    return TRUE;
    case SCAN_F10:  *LvKey = LV_KEY_ENTER;  return TRUE;
    case SCAN_NULL:
      //
      // Fall through to the UnicodeChar path below.
      //
      break;
    default:
      //
      // Unmapped scan code (e.g. F2-F8, INSERT, PAGE_UP/DOWN handled above,
      // extended codes from SimpleTextInEx).  Drop silently.
      //
      return FALSE;
  }

  //
  // 2. Map Unicode control / printable characters
  //
  switch (UnicodeChar) {
    case CHAR_CARRIAGE_RETURN:  *LvKey = LV_KEY_ENTER;      return TRUE;
    case CHAR_LINEFEED:         *LvKey = LV_KEY_ENTER;      return TRUE;
    case CHAR_BACKSPACE:        *LvKey = LV_KEY_BACKSPACE;  return TRUE;
    case CHAR_TAB:              *LvKey = LV_KEY_NEXT;       return TRUE;
    case L'\x00':
      //
      // Both ScanCode and UnicodeChar are zero: modifier-only event.
      // Nothing to forward.
      //
      return FALSE;
    default:
      //
      // Printable character — pass the Unicode code point directly.
      // lvgl treats key values >= 0x20 as literal character input.
      //
      *LvKey = (UINT32)UnicodeChar;
      return TRUE;
  }
}

// ---------------------------------------------------------------------------
// lvgl read callback
// ---------------------------------------------------------------------------

/**
  lvgl keypad read callback.

  Called periodically by lvgl's internal timer.  Drains one keystroke per
  invocation from the EFI keyboard buffer.  If a keystroke is available and
  successfully mapped, the key and PRESSED state are written into @a Data and
  `continue_reading` is set so lvgl will call again immediately to drain any
  remaining buffered keystrokes.

  When no keystroke is available (EFI_NOT_READY) or the keystroke does not
  map to a known lvgl key, the callback writes key = 0 and
  state = LV_INDEV_STATE_RELEASED.

  @param[in]  Indev   Pointer to the lvgl input device object (unused).
  @param[out] Data    Pointer to the data structure to fill.
**/
STATIC
VOID
KbReadCb (
  lv_indev_t       *Indev,
  lv_indev_data_t  *Data
  )
{
  EFI_STATUS    Status;
  EFI_KEY_DATA  KeyData;
  UINT32        LvKey;

  //
  // Default: no key pressed.
  //
  Data->key             = 0;
  Data->state           = LV_INDEV_STATE_RELEASED;
  Data->continue_reading = FALSE;

  if (mKb == NULL) {
    return;
  }

  Status = mKb->ReadKeyStrokeEx (mKb, &KeyData);
  if (Status == EFI_NOT_READY) {
    //
    // No keystroke buffered — nothing to report.
    //
    return;
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "LvglKb: ReadKeyStrokeEx error: %r\n", Status));
    return;
  }

  //
  // Attempt to map the EFI key to an lvgl key constant.
  //
  if (!MapEfiKeyToLvgl (&KeyData, &LvKey)) {
    //
    // Unmapped key — report released so lvgl state machine stays clean.
    //
    return;
  }

  Data->key   = LvKey;
  Data->state = LV_INDEV_STATE_PRESSED;

  //
  // Ask lvgl to call us again immediately so that any additional buffered
  // keystrokes are drained within the same tick rather than one per timer
  // period.
  //
  Data->continue_reading = TRUE;
}

// ---------------------------------------------------------------------------
// Public initialisation
// ---------------------------------------------------------------------------

/**
  Initialize keyboard input device.

  Steps performed:
  1. Locate EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL via gBS->LocateProtocol.
     If not found a warning is logged and mKb remains NULL (the indev is
     still registered so the application does not need to handle a NULL
     return from this function specially — it will simply report no keys).
  2. Create an lvgl input device with lv_indev_create().
  3. Set the type to LV_INDEV_TYPE_KEYPAD.
  4. Register KbReadCb as the read callback.

  @retval non-NULL   Pointer to the registered lv_indev_t.
  @retval NULL       lv_indev_create() allocation failed.
**/
lv_indev_t *
LvglKbInit (
  VOID
  )
{
  EFI_STATUS  Status;
  lv_indev_t  *Indev;

  // ------------------------------------------------------------------
  // 1. Locate SimpleTextInputEx
  // ------------------------------------------------------------------
  Status = gBS->LocateProtocol (
                  &gEfiSimpleTextInputExProtocolGuid,
                  NULL,
                  (VOID **)&mKb
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_WARN,
      "LvglKb: SimpleTextInputEx not found (%r); keyboard input disabled\n",
      Status
      ));
    mKb = NULL;
  }

  // ------------------------------------------------------------------
  // 2. Create lvgl input device
  // ------------------------------------------------------------------
  Indev = lv_indev_create ();
  if (Indev == NULL) {
    DEBUG ((DEBUG_ERROR, "LvglKb: lv_indev_create failed\n"));
    return NULL;
  }

  // ------------------------------------------------------------------
  // 3. Set type
  // ------------------------------------------------------------------
  lv_indev_set_type (Indev, LV_INDEV_TYPE_KEYPAD);

  // ------------------------------------------------------------------
  // 4. Register read callback
  // ------------------------------------------------------------------
  lv_indev_set_read_cb (Indev, KbReadCb);

  DEBUG ((
    DEBUG_INFO,
    "LvglKb: keyboard indev registered (protocol %a)\n",
    (mKb != NULL) ? "found" : "absent"
    ));

  return Indev;
}
