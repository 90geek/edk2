/** @file
  LvglLib public API declarations.

  Consumers of LvglLib include this header to obtain the three entry points:
    - UefiLvglInit()   — one-time initialisation (call from driver/app entry)
    - UefiLvglTick()   — periodic update loop body (call in the main loop)
    - UefiLvglDeinit() — cleanup on exit

  Copyright (c) 2024, EDK2 Contributors.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef LVGL_LIB_H_
#define LVGL_LIB_H_

#include <Uefi.h>

/**
  Initialise the lvgl library and all UEFI port backends.

  Must be called once before any other lvgl or LvglLib function.  The
  function performs the following steps in order:
    1. lv_init()
    2. LvglDispInit()  — locate GOP, allocate draw buffers, create display
    3. LvglKbInit()    — locate SimpleTextInputEx, register keypad indev
    4. LvglMouseInit() — locate SimplePointer, register pointer indev
    5. LvglTouchInit() — locate AbsolutePointer, register pointer indev

  If LvglDispInit() returns NULL (GOP not found or memory allocation
  failure) this function returns EFI_DEVICE_ERROR and no further
  initialisation is performed.  Keyboard, mouse, and touch failures are
  non-fatal: a warning is logged and the respective indev is disabled.

  @param[in] ImageHandle   Image handle of the calling driver or application.
  @param[in] SystemTable   Pointer to the EFI system table.

  @retval EFI_SUCCESS       All required backends initialised successfully.
  @retval EFI_DEVICE_ERROR  GOP display backend could not be initialised.
**/
EFI_STATUS
EFIAPI
UefiLvglInit (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

/**
  Advance the lvgl tick counter and run the lvgl timer/task handler.

  Must be called repeatedly from the application's main loop.  Each call:
    1. Stalls for 5 ms via gBS->Stall(5000).
    2. Advances the lvgl tick by 5 ms with lv_tick_inc(5).
    3. Processes all pending lvgl timers with lv_timer_handler().

  @retval (none)
**/
VOID
EFIAPI
UefiLvglTick (
  VOID
  );

/**
  De-initialise the lvgl library and release all resources.

  Calls lv_deinit() which destroys all displays, input devices, and
  internal lvgl state.  After this call, UefiLvglInit() may be invoked
  again to re-initialise from scratch.

  @retval (none)
**/
VOID
EFIAPI
UefiLvglDeinit (
  VOID
  );

#endif /* LVGL_LIB_H_ */
