/** @file
  lvgl pointer input adapters — SimplePointer (mouse) + AbsolutePointer (touch)

  Provides two separate lvgl POINTER input devices:
    - Mouse:  backed by EFI_SIMPLE_POINTER_PROTOCOL (relative movement)
    - Touch:  backed by EFI_ABSOLUTE_POINTER_PROTOCOL (absolute coordinates)

  Both adapters are polled every time lvgl calls the read callback from its
  internal timer.  If the underlying protocol is unavailable the indev is
  still created but will permanently report RELEASED at (0, 0), so callers
  do not need to handle a NULL return specially.

  Copyright (c) 2024, EDK2 Contributors.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "LvglPort.h"

// ---------------------------------------------------------------------------
// Module-level globals
// ---------------------------------------------------------------------------

/** SimplePointer protocol handle.  NULL if unavailable. **/
STATIC EFI_SIMPLE_POINTER_PROTOCOL    *mMouse = NULL;

/** Accumulated mouse cursor X position in pixels. **/
STATIC INT32                           mMouseX = 0;

/** Accumulated mouse cursor Y position in pixels. **/
STATIC INT32                           mMouseY = 0;

/** AbsolutePointer protocol handle.  NULL if unavailable. **/
STATIC EFI_ABSOLUTE_POINTER_PROTOCOL  *mTouch = NULL;

// ---------------------------------------------------------------------------
// Mouse read callback (SimplePointer → lvgl POINTER)
// ---------------------------------------------------------------------------

/**
  lvgl pointer read callback for the mouse input device.

  Called periodically by lvgl's internal timer.  Reads one state snapshot
  from EFI_SIMPLE_POINTER_PROTOCOL, converts the relative movement counts
  to pixel deltas using the device resolution, accumulates the cursor
  position, and clamps it to the screen bounds reported by GOP.

  @param[in]  Indev   Pointer to the lvgl input device object (unused).
  @param[out] Data    Pointer to the data structure to fill.
**/
STATIC
VOID
MouseReadCb (
  lv_indev_t       *Indev,
  lv_indev_data_t  *Data
  )
{
  EFI_STATUS              Status;
  EFI_SIMPLE_POINTER_STATE  State;
  UINT32                  ScreenW;
  UINT32                  ScreenH;
  INT32                   DeltaX;
  INT32                   DeltaY;

  //
  // Default: report current (possibly previous) position, no button pressed.
  //
  Data->point.x = (int32_t)mMouseX;
  Data->point.y = (int32_t)mMouseY;
  Data->state   = LV_INDEV_STATE_RELEASED;

  if (mMouse == NULL) {
    return;
  }

  Status = mMouse->GetState (mMouse, &State);
  if (Status == EFI_NOT_READY) {
    //
    // No movement since last poll — just report current position.
    //
    return;
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "LvglMouse: GetState error: %r\n", Status));
    return;
  }

  //
  // Determine screen dimensions from the shared GOP pointer.
  //
  if (gGop == NULL) {
    return;
  }

  ScreenW = gGop->Mode->Info->HorizontalResolution;
  ScreenH = gGop->Mode->Info->VerticalResolution;

  //
  // Convert relative counts to pixel deltas.
  //
  // EFI_SIMPLE_POINTER_MODE::ResolutionX is in counts/mm.
  // The raw RelativeMovementX value is in counts.
  // pixel_delta = counts / (counts/mm) = mm — but we want pixels.
  // A common heuristic is to treat 1 mm as 1 pixel for UEFI environments.
  // Guard against division by zero for devices with ResolutionX == 0.
  //
  if (mMouse->Mode->ResolutionX != 0) {
    DeltaX = (INT32)(State.RelativeMovementX / (INT64)mMouse->Mode->ResolutionX);
  } else {
    DeltaX = (INT32)State.RelativeMovementX;
  }

  if (mMouse->Mode->ResolutionY != 0) {
    DeltaY = (INT32)(State.RelativeMovementY / (INT64)mMouse->Mode->ResolutionY);
  } else {
    DeltaY = (INT32)State.RelativeMovementY;
  }

  //
  // Accumulate position and clamp to screen bounds [0, ScreenW-1] x [0, ScreenH-1].
  //
  mMouseX += DeltaX;
  mMouseY += DeltaY;

  if (mMouseX < 0) {
    mMouseX = 0;
  } else if ((UINT32)mMouseX >= ScreenW) {
    mMouseX = (INT32)(ScreenW - 1U);
  }

  if (mMouseY < 0) {
    mMouseY = 0;
  } else if ((UINT32)mMouseY >= ScreenH) {
    mMouseY = (INT32)(ScreenH - 1U);
  }

  Data->point.x = (int32_t)mMouseX;
  Data->point.y = (int32_t)mMouseY;
  Data->state   = State.LeftButton
                    ? LV_INDEV_STATE_PRESSED
                    : LV_INDEV_STATE_RELEASED;
}

// ---------------------------------------------------------------------------
// Touch read callback (AbsolutePointer → lvgl POINTER)
// ---------------------------------------------------------------------------

/**
  lvgl pointer read callback for the touch input device.

  Called periodically by lvgl's internal timer.  Reads one state snapshot
  from EFI_ABSOLUTE_POINTER_PROTOCOL and converts the absolute coordinates
  to screen-space pixels using linear normalisation against the device range
  reported in EFI_ABSOLUTE_POINTER_MODE.

  Touch is considered active when EFI_ABSP_TouchActive is set in
  State.ActiveButtons.

  @param[in]  Indev   Pointer to the lvgl input device object (unused).
  @param[out] Data    Pointer to the data structure to fill.
**/
STATIC
VOID
TouchReadCb (
  lv_indev_t       *Indev,
  lv_indev_data_t  *Data
  )
{
  EFI_STATUS                 Status;
  EFI_ABSOLUTE_POINTER_STATE State;
  UINT64                     MaxX;
  UINT64                     MaxY;
  UINT32                     ScreenW;
  UINT32                     ScreenH;
  INT32                      PixelX;
  INT32                      PixelY;

  //
  // Default: not touched at origin.
  //
  Data->point.x = 0;
  Data->point.y = 0;
  Data->state   = LV_INDEV_STATE_RELEASED;

  if (mTouch == NULL) {
    return;
  }

  Status = mTouch->GetState (mTouch, &State);
  if (Status == EFI_NOT_READY) {
    //
    // Touch state has not changed since last poll.
    //
    return;
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "LvglTouch: GetState error: %r\n", Status));
    return;
  }

  if (gGop == NULL) {
    return;
  }

  ScreenW = gGop->Mode->Info->HorizontalResolution;
  ScreenH = gGop->Mode->Info->VerticalResolution;

  //
  // Normalise absolute device coordinates to screen pixels:
  //   pixel_x = CurrentX * ScreenW / (MaxX + 1)
  //
  // Guard against division by zero when the device range is not reported
  // (both AbsoluteMinX and AbsoluteMaxX being 0 indicates no x-axis support).
  //
  MaxX = mTouch->Mode->AbsoluteMaxX;
  MaxY = mTouch->Mode->AbsoluteMaxY;

  if (MaxX != 0) {
    PixelX = (INT32)(State.CurrentX * (UINT64)ScreenW / (MaxX + 1ULL));
  } else {
    PixelX = 0;
  }

  if (MaxY != 0) {
    PixelY = (INT32)(State.CurrentY * (UINT64)ScreenH / (MaxY + 1ULL));
  } else {
    PixelY = 0;
  }

  //
  // Clamp to screen bounds for safety.
  //
  if (PixelX < 0) {
    PixelX = 0;
  } else if ((UINT32)PixelX >= ScreenW) {
    PixelX = (INT32)(ScreenW - 1U);
  }

  if (PixelY < 0) {
    PixelY = 0;
  } else if ((UINT32)PixelY >= ScreenH) {
    PixelY = (INT32)(ScreenH - 1U);
  }

  Data->point.x = (int32_t)PixelX;
  Data->point.y = (int32_t)PixelY;

  //
  // EFI_ABSP_TouchActive (0x00000001) is set when the touch sensor is active.
  //
  Data->state = ((State.ActiveButtons & EFI_ABSP_TouchActive) != 0)
                  ? LV_INDEV_STATE_PRESSED
                  : LV_INDEV_STATE_RELEASED;
}

// ---------------------------------------------------------------------------
// Public initialisation — mouse
// ---------------------------------------------------------------------------

/**
  Initialize mouse input device (SimplePointer → lvgl POINTER indev).

  Steps performed:
  1. Locate EFI_SIMPLE_POINTER_PROTOCOL via gBS->LocateProtocol.
     If not found, a warning is logged and mMouse remains NULL (the indev
     is still registered; it will permanently report no movement).
  2. Create an lvgl input device with lv_indev_create().
  3. Set the type to LV_INDEV_TYPE_POINTER.
  4. Register MouseReadCb as the read callback.

  @retval non-NULL   Pointer to the registered lv_indev_t.
  @retval NULL       lv_indev_create() allocation failed.
**/
lv_indev_t *
LvglMouseInit (
  VOID
  )
{
  EFI_STATUS  Status;
  lv_indev_t  *Indev;

  // ------------------------------------------------------------------
  // 1. Locate SimplePointer
  // ------------------------------------------------------------------
  Status = gBS->LocateProtocol (
                  &gEfiSimplePointerProtocolGuid,
                  NULL,
                  (VOID **)&mMouse
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_WARN,
      "LvglMouse: SimplePointer not found (%r); mouse input disabled\n",
      Status
      ));
    mMouse = NULL;
  }

  // ------------------------------------------------------------------
  // 2. Create lvgl input device
  // ------------------------------------------------------------------
  Indev = lv_indev_create ();
  if (Indev == NULL) {
    DEBUG ((DEBUG_ERROR, "LvglMouse: lv_indev_create failed\n"));
    return NULL;
  }

  // ------------------------------------------------------------------
  // 3. Set type
  // ------------------------------------------------------------------
  lv_indev_set_type (Indev, LV_INDEV_TYPE_POINTER);

  // ------------------------------------------------------------------
  // 4. Register read callback
  // ------------------------------------------------------------------
  lv_indev_set_read_cb (Indev, MouseReadCb);

  DEBUG ((
    DEBUG_INFO,
    "LvglMouse: mouse indev registered (protocol %a)\n",
    (mMouse != NULL) ? "found" : "absent"
    ));

  return Indev;
}

// ---------------------------------------------------------------------------
// Public initialisation — touch
// ---------------------------------------------------------------------------

/**
  Initialize touch input device (AbsolutePointer → lvgl POINTER indev).

  Steps performed:
  1. Locate EFI_ABSOLUTE_POINTER_PROTOCOL via gBS->LocateProtocol.
     If not found, a warning is logged and mTouch remains NULL (the indev
     is still registered; it will permanently report no touch events).
  2. Create an lvgl input device with lv_indev_create().
  3. Set the type to LV_INDEV_TYPE_POINTER.
  4. Register TouchReadCb as the read callback.

  @retval non-NULL   Pointer to the registered lv_indev_t.
  @retval NULL       lv_indev_create() allocation failed.
**/
lv_indev_t *
LvglTouchInit (
  VOID
  )
{
  EFI_STATUS  Status;
  lv_indev_t  *Indev;

  // ------------------------------------------------------------------
  // 1. Locate AbsolutePointer
  // ------------------------------------------------------------------
  Status = gBS->LocateProtocol (
                  &gEfiAbsolutePointerProtocolGuid,
                  NULL,
                  (VOID **)&mTouch
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_WARN,
      "LvglTouch: AbsolutePointer not found (%r); touch input disabled\n",
      Status
      ));
    mTouch = NULL;
  }

  // ------------------------------------------------------------------
  // 2. Create lvgl input device
  // ------------------------------------------------------------------
  Indev = lv_indev_create ();
  if (Indev == NULL) {
    DEBUG ((DEBUG_ERROR, "LvglTouch: lv_indev_create failed\n"));
    return NULL;
  }

  // ------------------------------------------------------------------
  // 3. Set type
  // ------------------------------------------------------------------
  lv_indev_set_type (Indev, LV_INDEV_TYPE_POINTER);

  // ------------------------------------------------------------------
  // 4. Register read callback
  // ------------------------------------------------------------------
  lv_indev_set_read_cb (Indev, TouchReadCb);

  DEBUG ((
    DEBUG_INFO,
    "LvglTouch: touch indev registered (protocol %a)\n",
    (mTouch != NULL) ? "found" : "absent"
    ));

  return Indev;
}
