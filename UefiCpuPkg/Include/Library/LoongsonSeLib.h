/** @file

Base CPU security Library for Loongarch

Copyright (c) 2026, Loongson Technology Corporation Limited. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef LOONGSON_SE_LIB_H_
#define LOONGSON_SE_LIB_H_

#include <Uefi/UefiBaseType.h>

/**
  Send a TPM command to the Loongson SE device.

  @param[in]      In      Pointer to the TPM command input buffer.
  @param[in]      InLen   Size of the input buffer in bytes.
  @param[out]     Out     Pointer to the TPM command output buffer.
  @param[in,out]  OutLen  On input, size of the output buffer in bytes.
                         On output, actual size of the response data in bytes.

  @retval EFI_SUCCESS            The TPM command was sent successfully.
  @retval EFI_INVALID_PARAMETER  One or more input parameters are invalid.
  @retval EFI_DEVICE_ERROR       The SE device failed to process the command.
  @retval EFI_NOT_FOUND          The SE device was not detected.
  @retval EFI_NOT_READY          The SE device was not init.
  @retval EFI_UNSUPPORTED        This interface is not supported.
**/
EFI_STATUS
EFIAPI
LoongsonSendTpmCmd (
  IN     UINT8   *In,
  IN     UINT32  InLen,
  OUT    UINT8   *Out,
  IN OUT UINT32  *OutLen
  );

/**
  Generate random data by using the Loongson SE RNG engine.

  @param[in]   Src   Pointer to the optional input source buffer.
  @param[in]   Slen  Size of the input source buffer in bytes.
  @param[out]  Dstn  Pointer to the random output buffer.
  @param[in]   Dlen  Size of the random output buffer in bytes.

  @retval EFI_SUCCESS            Random data was generated successfully.
  @retval EFI_INVALID_PARAMETER  One or more input parameters are invalid.
  @retval EFI_LOAD_ERROR         The RNG engine failed to generate random data.
  @retval EFI_UNSUPPORTED        This interface is not supported.
**/
EFI_STATUS
EFIAPI
LoongsonRngGenerate (
  IN  UINT8   *Src,
  IN  UINT32  Slen,
  OUT UINT8   *Dstn,
  IN  UINT32  Dlen
  );

/**
  Seed the Loongson SE RNG engine.

  @param[in]  Seed  Pointer to the seed buffer.
  @param[in]  Slen  Size of the seed buffer in bytes.

  @retval EFI_SUCCESS            The RNG seed was set successfully.
  @retval EFI_INVALID_PARAMETER  The seed buffer is invalid.
  @retval EFI_LOAD_ERROR         The SE device failed to set the RNG seed.
  @retval EFI_UNSUPPORTED        This interface is not supported.
**/
EFI_STATUS
EFIAPI
LoongsonRngSeed (
  IN UINT8   *Seed,
  IN UINT32  Slen
  );

/**
  Probe whether the Loongson SE TPM function is available.

  @retval EFI_SUCCESS       The TPM function is available.
  @retval EFI_NOT_FOUND     The TPM function was not found.
  @retval EFI_DEVICE_ERROR  The TPM function is not working correctly.
  @retval EFI_UNSUPPORTED   This interface is not supported.
**/
EFI_STATUS
EFIAPI
LoongsonTpmProbe (
  VOID
  );

/**
  Detect whether the Loongson SE device exists.

  @retval EFI_SUCCESS    The SE device was detected.
  @retval EFI_NOT_FOUND  The SE device was not detected.
  @retval EFI_NOT_READY  The SE device was not init.
**/

/**
  Probe whether the Loongson SE RNG engine is available.

  @retval EFI_SUCCESS       The RNG engine is available.
  @retval EFI_NOT_FOUND     The RNG engine was not found.
  @retval EFI_DEVICE_ERROR  The RNG engine is not working correctly.
  @retval EFI_UNSUPPORTED   This interface is not supported.
**/
EFI_STATUS
EFIAPI
LoongsonRngProbe (
  VOID
  );

/**
  Initialize the Loongson SE device.

  @retval EFI_SUCCESS       The SE device was initialized successfully.
  @retval EFI_NOT_FOUND     The SE device was not found.
  @retval EFI_DEVICE_ERROR  The SE device initialization failed.
  @retval EFI_UNSUPPORTED   This interface is not supported.
**/
EFI_STATUS
EFIAPI
LoongsonSeDeviceInit (
  VOID
  );

/**
  Detect whether the Loongson SE device exists.

  @retval EFI_SUCCESS    The SE device was detected.
  @retval EFI_NOT_FOUND  The SE device was not detected.
  @retval EFI_NOT_READY    The SE disable or run failed.
  @retval EFI_UNSUPPORTED  This interface is not supported.
**/
EFI_STATUS
EFIAPI
LoongsonSeDetect (
  VOID
  );

/**
  Get the SE device mode state.

  @retval TRUE  SE is device mode.
  @retval FALSE  SE is not device mode.
**/
BOOLEAN
EFIAPI
LsSeIsDevMode(
  VOID
  );

/**
  Give a DDR memory region to the SE device.

  @param[in]  Base  Base address of the DDR memory region.
  @param[in]  Size  Size of the DDR memory region in bytes.

  @retval EFI_SUCCESS            The DDR memory region was given successfully.
  @retval EFI_UNSUPPORTED        The SE is device mode or interface is not supported.
  @retval EFI_NOT_READY          The SE disable or run failed.
  @retval EFI_DEVICE_ERROR       The SE device failed to accept the DDR region.
**/
EFI_STATUS
EFIAPI
GiveSeDdr (
  IN UINT64  Base,
  IN UINT64  Size
  );

/**
  Burn data into SE OTP.

  @param[in]  OtpBase   Base address of the OTP data buffer.
  @param[in]  Len       Size of the OTP data in bytes.
  @param[in]  OtpIndex  OTP index to be programmed.

  @retval EFI_SUCCESS      The SE otp burn done.
  @retval EFI_UNSUPPORTED  This interface is not supported.
**/
VOID
EFIAPI
SeBurnOtp (
  IN UINT64  OtpBase,
  IN UINT64  Len,
  IN UINT32  OtpIndex
  );

/**
  Burn data into SE flash.

  @param[in]  DataBase  Base address of the flash data buffer.
  @param[in]  Len       Size of the flash data in bytes.

  @retval EFI_SUCCESS      The SE flash burn done.
  @retval EFI_UNSUPPORTED  This interface is not supported.
**/
VOID
EFIAPI
SeBurnFlash (
  IN UINT64  DataBase,
  IN UINT64  Len
  );

#endif // LOONGSON_SE_LIB_H_
