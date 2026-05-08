/** @file

Base CPU security Library for Loongarch

Copyright (c) 2026, Loongson Technology Corporation Limited. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/



#include <Library/DebugLib.h>
#include <Uefi/UefiBaseType.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/BaseLib.h>
#include <Io.h>

/**
  Send a TPM command to the Loongson SE device.

  @param[in]      In      Pointer to the TPM command input buffer.
  @param[in]      InLen   Size of the input buffer in bytes.
  @param[out]     Out     Pointer to the TPM command output buffer.
  @param[in,out]  OutLen  On input, size of the output buffer in bytes.
                         On output, actual size of the response data in bytes.

  @retval EFI_UNSUPPORTED  This interface is not supported.
**/
EFI_STATUS
EFIAPI
LoongsonSendTpmCmd (
  IN     UINT8   *In,
  IN     UINT32  InLen,
  OUT    UINT8   *Out,
  IN OUT UINT32  *OutLen
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Generate random data by using the Loongson SE RNG engine.

  @param[in]   Src   Pointer to the optional input source buffer.
  @param[in]   Slen  Size of the input source buffer in bytes.
  @param[out]  Dstn  Pointer to the random output buffer.
  @param[in]   Dlen  Size of the random output buffer in bytes.

  @retval EFI_UNSUPPORTED  This interface is not supported.
**/
EFI_STATUS
EFIAPI
LoongsonRngGenerate (
  IN  UINT8   *Src,
  IN  UINT32  Slen,
  OUT UINT8   *Dstn,
  IN  UINT32  Dlen
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Seed the Loongson SE RNG engine.

  @param[in]  Seed  Pointer to the seed buffer.
  @param[in]  Slen  Size of the seed buffer in bytes.

  @retval EFI_UNSUPPORTED  This interface is not supported.
**/
EFI_STATUS
EFIAPI
LoongsonRngSeed (
  IN UINT8   *Seed,
  IN UINT32  Slen
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Probe whether the Loongson SE RNG engine is available.

  @retval EFI_UNSUPPORTED  This interface is not supported.
**/
EFI_STATUS
EFIAPI
LoongsonRngProbe (
  VOID
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Probe whether the Loongson SE TPM function is available.

  @retval EFI_UNSUPPORTED  This interface is not supported.
**/
EFI_STATUS
EFIAPI
LoongsonTpmProbe (
  VOID
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Initialize the Loongson SE device.

  @retval EFI_UNSUPPORTED  This interface is not supported.
**/
EFI_STATUS
EFIAPI
LoongsonSeDeviceInit (
  VOID
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Detect whether the Loongson SE device exists.

  @retval EFI_UNSUPPORTED  This interface is not supported.
**/
EFI_STATUS
EFIAPI
LoongsonSeDetect (
  VOID
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Enable SE read access.

  @retval EFI_UNSUPPORTED  This interface is not supported.
**/
EFI_STATUS
EFIAPI
SeReadEnable (
  VOID
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Detect the SE device and return the raw detect result.

  @retval 1  The SE device exists.
  @retval 0  The SE device does not exist.

  @note This stub implementation always returns 1.
**/
UINT8
EFIAPI
SeDetect (
  VOID
  )
{
  return 1;
}

/**
  Give a DDR memory region to the SE device.

  @param[in]  Base  Base address of the DDR memory region.
  @param[in]  Size  Size of the DDR memory region in bytes.

  @retval EFI_UNSUPPORTED  This interface is not supported.
**/
EFI_STATUS
EFIAPI
GiveSeDdr (
  IN UINT64  Base,
  IN UINT64  Size
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Get the SE development mode state.

  @retval 1  SE development mode is enabled.
  @retval 0  SE development mode is disabled.

  @note This stub implementation always returns 0.
**/
UINT8
EFIAPI
GetSeDevMode (
  VOID
  )
{
  return 0;
}

/**
  Burn data into SE OTP.

  @param[in]  OtpBase   Base address of the OTP data buffer.
  @param[in]  Len       Size of the OTP data in bytes.
  @param[in]  OtpIndex  OTP index to be programmed.

  @retval None.

  @note This stub implementation does nothing.
**/
VOID
EFIAPI
SeBurnOtp (
  IN UINT64  OtpBase,
  IN UINT64  Len,
  IN UINT32  OtpIndex
  )
{
  return;
}

/**
  Burn data into SE flash.

  @param[in]  DataBase  Base address of the flash data buffer.
  @param[in]  Len       Size of the flash data in bytes.

  @retval None.

  @note This stub implementation does nothing.
**/
VOID
EFIAPI
SeBurnFlash (
  IN UINT64  DataBase,
  IN UINT64  Len
  )
{
  return;
}
