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
  Get the SE device mode state.

  @retval TRUE  SE is device mode.
  @retval FALSE  SE is not device mode.
**/
BOOLEAN
EFIAPI
LsSeIsDevMode(
  VOID
  )
{
  return FALSE;
}

/**
  Burn data into SE OTP.

  @param[in]  OtpBase   Base address of the OTP data buffer.
  @param[in]  Len       Size of the OTP data in bytes.
  @param[in]  OtpIndex  OTP index to be programmed.

  @retval EFI_UNSUPPORTED  This interface is not supported.
**/
EFI_STATUS
EFIAPI
SeBurnOtp (
  IN UINT64  OtpBase,
  IN UINT64  Len,
  IN UINT32  OtpIndex
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Burn data into SE flash.

  @param[in]  DataBase  Base address of the flash data buffer.
  @param[in]  Len       Size of the flash data in bytes.

  @retval EFI_UNSUPPORTED  This interface is not supported.
**/
EFI_STATUS
EFIAPI
SeBurnFlash (
  IN UINT64  DataBase,
  IN UINT64  Len
  )
{
  return EFI_UNSUPPORTED;
}
