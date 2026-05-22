/** @file
  lvgl GOP display backend for UEFI/EDK2.

  Maps the lvgl v9.2.2 display flush callback to EFI_GRAPHICS_OUTPUT_PROTOCOL
  Blt() calls so that every dirty rectangle rendered by lvgl is pushed directly
  to the GOP frame-buffer.

  Pixel-format notes
  ------------------
  With LV_COLOR_DEPTH == 32 the lvgl native color format is XRGB8888.
  The in-memory layout of lv_color32_t is {blue, green, red, alpha/pad},
  i.e. [B][G][R][X] per pixel.

  EFI_GRAPHICS_OUTPUT_BLT_PIXEL is defined as {Blue, Green, Red, Reserved},
  i.e. the same [B][G][R][X] byte order.

  Therefore NO byte-swap is required in the flush callback; the lvgl draw
  buffer can be passed directly to Blt() with EfiBltBufferToVideo.

  Copyright (c) 2024, EDK2 Contributors.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "LvglPort.h"

// ---------------------------------------------------------------------------
// Module-level globals
// ---------------------------------------------------------------------------

/** GOP protocol handle - also used by other port modules (declared in LvglPort.h). **/
EFI_GRAPHICS_OUTPUT_PROTOCOL  *gGop = NULL;

// ---------------------------------------------------------------------------
// Flush callback
// ---------------------------------------------------------------------------

/**
  lvgl flush callback — copy the rendered rectangle to the GOP frame-buffer.

  lvgl calls this function after rendering each dirty rectangle.  The pixel
  data in @a px_map follows the display's color format (XRGB8888 / 32 bpp).
  The memory layout of one pixel is [B][G][R][X], identical to
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL, so no byte-reordering is necessary.

  @param[in] Disp    Pointer to the lvgl display object.
  @param[in] Area    Dirty rectangle in display coordinates (inclusive bounds).
  @param[in] PxMap   Pointer to the pixel buffer produced by the renderer.
**/
STATIC
VOID
DispFlushCb (
  lv_display_t       *Disp,
  const lv_area_t    *Area,
  uint8_t            *PxMap
  )
{
  EFI_STATUS  Status;
  INT32       Width;
  INT32       Height;

  if ((gGop == NULL) || (Area == NULL) || (PxMap == NULL)) {
    lv_display_flush_ready (Disp);
    return;
  }

  Width  = Area->x2 - Area->x1 + 1;
  Height = Area->y2 - Area->y1 + 1;

  if ((Width <= 0) || (Height <= 0)) {
    lv_display_flush_ready (Disp);
    return;
  }

  //
  // EfiBltBufferToVideo: copy a rectangle from the caller-supplied buffer to
  // the video frame-buffer.
  //
  // BltBuffer  - source pixel array (row-major, no padding between rows in the
  //              dirty rectangle itself, but the stride in the source image
  //              equals the display width).
  // SourceX / SourceY - top-left of the region inside BltBuffer.
  //              When PARTIAL render mode is used lvgl provides a buffer whose
  //              width equals (Area->x2 - Area->x1 + 1), so SourceX/Y = 0.
  // DestinationX / DestinationY - top-left on the physical screen.
  // Width / Height - rectangle size.
  // Delta - bytes per row of the *source* buffer; 0 means contiguous rows
  //         (Width * sizeof pixel), which matches what lvgl gives us.
  //
  Status = gGop->Blt (
                   gGop,
                   (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)(UINTN)PxMap,
                   EfiBltBufferToVideo,
                   0,                          // SourceX
                   0,                          // SourceY
                   (UINTN)Area->x1,            // DestinationX
                   (UINTN)Area->y1,            // DestinationY
                   (UINTN)Width,
                   (UINTN)Height,
                   0                           // Delta (contiguous rows)
                   );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "LvglDisp: Blt failed: %r\n", Status));
  }

  // Signal to lvgl that flushing is complete (synchronous driver).
  lv_display_flush_ready (Disp);
}

// ---------------------------------------------------------------------------
// Public initialisation
// ---------------------------------------------------------------------------

/**
  Initialize GOP display backend and create an lvgl display object.

  Steps performed:
  1. Locate EFI_GRAPHICS_OUTPUT_PROTOCOL via gBS->LocateProtocol.
  2. Read current mode (HorizontalResolution, VerticalResolution).
  3. Allocate two partial draw buffers (each width * 10 rows * 4 bytes/pixel).
  4. Create an lv_display_t with the detected resolution.
  5. Register DispFlushCb as the flush callback.
  6. Hand the buffers to lvgl with PARTIAL render mode.

  @param[in] ImageHandle   Caller image handle (unused, reserved for future use).
  @param[in] SystemTable   EFI system table pointer (unused, gBS is already set).

  @retval non-NULL   Pointer to the initialised lv_display_t.
  @retval NULL       GOP not found or memory allocation failed.
**/
lv_display_t *
LvglDispInit (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS              Status;
  EFI_GUID                GopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
  UINT32                  HRes;
  UINT32                  VRes;
  UINT32                  BufSize;
  VOID                    *Buf1;
  VOID                    *Buf2;
  lv_display_t            *Disp;

  // ------------------------------------------------------------------
  // 1. Locate GOP
  // ------------------------------------------------------------------
  Status = gBS->LocateProtocol (&GopGuid, NULL, (VOID **)&gGop);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "LvglDisp: GOP not found: %r\n", Status));
    return NULL;
  }

  // ------------------------------------------------------------------
  // 2. Query current mode resolution
  // ------------------------------------------------------------------
  HRes = gGop->Mode->Info->HorizontalResolution;
  VRes = gGop->Mode->Info->VerticalResolution;

  DEBUG ((
    DEBUG_INFO,
    "LvglDisp: GOP resolution %u x %u\n",
    (UINT32)HRes,
    (UINT32)VRes
    ));

  // ------------------------------------------------------------------
  // 3. Allocate two partial draw buffers
  //    Each buffer covers width * 10 rows * 4 bytes/pixel.
  //    This matches the lvgl recommendation of at least 1/10 screen.
  // ------------------------------------------------------------------
  BufSize = HRes * 10U * sizeof (UINT32);

  Buf1 = AllocatePool (BufSize);
  if (Buf1 == NULL) {
    DEBUG ((DEBUG_ERROR, "LvglDisp: AllocatePool Buf1 failed\n"));
    return NULL;
  }

  Buf2 = AllocatePool (BufSize);
  if (Buf2 == NULL) {
    DEBUG ((DEBUG_ERROR, "LvglDisp: AllocatePool Buf2 failed\n"));
    FreePool (Buf1);
    return NULL;
  }

  // ------------------------------------------------------------------
  // 4. Create the lvgl display object
  // ------------------------------------------------------------------
  Disp = lv_display_create ((int32_t)HRes, (int32_t)VRes);
  if (Disp == NULL) {
    DEBUG ((DEBUG_ERROR, "LvglDisp: lv_display_create failed\n"));
    FreePool (Buf1);
    FreePool (Buf2);
    return NULL;
  }

  // ------------------------------------------------------------------
  // 5. Register flush callback
  // ------------------------------------------------------------------
  lv_display_set_flush_cb (Disp, DispFlushCb);

  // ------------------------------------------------------------------
  // 6. Provide draw buffers with PARTIAL render mode
  //
  //    lv_display_set_buffers() signature (v9.2.2):
  //      void lv_display_set_buffers(lv_display_t *disp,
  //                                  void         *buf1,
  //                                  void         *buf2,
  //                                  uint32_t      buf_size,
  //                                  lv_display_render_mode_t render_mode);
  // ------------------------------------------------------------------
  lv_display_set_buffers (
    Disp,
    Buf1,
    Buf2,
    BufSize,
    LV_DISPLAY_RENDER_MODE_PARTIAL
    );

  DEBUG ((DEBUG_INFO, "LvglDisp: display initialised (%u x %u)\n", HRes, VRes));
  return Disp;
}
