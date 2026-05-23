/** @file
  LvglLib public API implementation — init, tick, and deinit.

  Provides three functions that together form the lifecycle of an lvgl-based
  UEFI application:

    UefiLvglInit()   — initialise lvgl and all UEFI port backends
    UefiLvglTick()   — advance time and run the lvgl task handler (main loop)
    UefiLvglDeinit() — tear down lvgl and release all resources

  Copyright (c) 2024, EDK2 Contributors.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "port/LvglPort.h"
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

/**
  Initialise the lvgl library and all UEFI port backends.

  Steps:
  1. lv_init()           — initialise lvgl core
  2. LvglDispInit()      — GOP display backend (required)
  3. LvglKbInit()        — keyboard input (optional)
  4. LvglMouseInit()     — mouse input (optional)
  5. LvglTouchInit()     — touch input (optional)

  @param[in] ImageHandle   Image handle of the calling driver or application.
  @param[in] SystemTable   Pointer to the EFI system table.

  @retval EFI_SUCCESS       Initialisation completed successfully.
  @retval EFI_DEVICE_ERROR  GOP display backend could not be initialised.
**/
EFI_STATUS
EFIAPI
UefiLvglInit (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  lv_display_t  *Disp;

  // ------------------------------------------------------------------
  // 1. Initialise lvgl core
  // ------------------------------------------------------------------
  lv_init ();

  // ------------------------------------------------------------------
  // 2. Initialise GOP display backend (mandatory)
  // ------------------------------------------------------------------
  Disp = LvglDispInit (ImageHandle, SystemTable);
  if (Disp == NULL) {
    DEBUG ((DEBUG_ERROR, "LvglLib: LvglDispInit failed — GOP not available\n"));
    return EFI_DEVICE_ERROR;
  }

  // ------------------------------------------------------------------
  // 3. Initialise keyboard input (non-fatal if unavailable)
  // ------------------------------------------------------------------
  LvglKbInit ();

  // ------------------------------------------------------------------
  // 4. Initialise mouse input (non-fatal if unavailable)
  // ------------------------------------------------------------------
  LvglMouseInit ();

  // ------------------------------------------------------------------
  // 5. Initialise touch input (non-fatal if unavailable)
  // ------------------------------------------------------------------
  LvglTouchInit ();

  DEBUG ((DEBUG_INFO, "LvglLib: UefiLvglInit complete\n"));
  return EFI_SUCCESS;
}

/**
  Advance the lvgl tick counter and run the lvgl timer/task handler.

  Intended to be called in a tight loop from the application's main event
  loop.  Each invocation:
    1. Stalls for 5 ms so that lvgl time accounting is accurate.
    2. Advances the lvgl tick by 5 ms.
    3. Runs all pending lvgl timers (animation, indev polling, rendering).
**/
VOID
EFIAPI
UefiLvglTick (
  VOID
  )
{
  //
  // 1. Wait 5 ms (5 000 µs).  gBS->Stall() takes microseconds.
  //
  gBS->Stall (5000);

  //
  // 2. Inform lvgl that 5 ms have passed since the previous tick.
  //
  lv_tick_inc (5);

  //
  // 3. Run all pending lvgl timers: input device polling, animations,
  //    screen invalidation, and display flushing.
  //
  lv_timer_handler ();
}

/**
  De-initialise the lvgl library and release all resources.

  Calls lv_deinit() which destroys all displays, input devices, widgets,
  and internal lvgl state.  After this call the library may be
  re-initialised with UefiLvglInit().
**/
VOID
EFIAPI
UefiLvglDeinit (
  VOID
  )
{
  lv_deinit ();
  DEBUG ((DEBUG_INFO, "LvglLib: UefiLvglDeinit complete\n"));
}
