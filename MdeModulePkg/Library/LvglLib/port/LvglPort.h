/** @file
  LvglLib internal port layer declarations.

  Provides initialization prototypes for GOP display backend, keyboard,
  mouse, and touch input devices.  All port files include this header.

  Copyright (c) 2024, EDK2 Contributors.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Uefi.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/SimpleTextInputEx.h>
#include <Protocol/SimplePointer.h>
#include <Protocol/AbsolutePointer.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>

// Include lvgl main header - path relative to LvglLib root (where lv_conf.h lives)
#include "lvgl/lvgl.h"

/**
  Initialize GOP display backend and create an lvgl display object.

  Locates EFI_GRAPHICS_OUTPUT_PROTOCOL, allocates draw buffers, creates an
  lv_display_t with PARTIAL render mode, and registers the GOP Blt flush
  callback.

  @param[in] ImageHandle   Image handle of the calling driver/application.
  @param[in] SystemTable   Pointer to the EFI system table.

  @retval non-NULL   Pointer to the newly created lv_display_t.
  @retval NULL       GOP not found or memory allocation failed.
**/
lv_display_t *
LvglDispInit (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

/**
  Initialize keyboard input device.

  @retval non-NULL   Pointer to the registered lv_indev_t.
  @retval NULL       SimpleTextInputEx not available.
**/
lv_indev_t *
LvglKbInit (
  VOID
  );

/**
  Initialize mouse input device (SimplePointer protocol).

  @retval non-NULL   Pointer to the registered lv_indev_t.
  @retval NULL       SimplePointer not available.
**/
lv_indev_t *
LvglMouseInit (
  VOID
  );

/**
  Initialize touch input device (AbsolutePointer protocol).

  @retval non-NULL   Pointer to the registered lv_indev_t.
  @retval NULL       AbsolutePointer not available.
**/
lv_indev_t *
LvglTouchInit (
  VOID
  );

/** GOP protocol global pointer — shared across port files. **/
extern EFI_GRAPHICS_OUTPUT_PROTOCOL  *gGop;
