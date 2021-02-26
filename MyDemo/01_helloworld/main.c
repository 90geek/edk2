/*main.c */
#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>

EFI_STATUS
EFIAPI
UefiMain (
          IN EFI_HANDLE        ImageHandle,
          IN EFI_SYSTEM_TABLE  *SystemTable
          )
{
   SystemTable -> ConOut-> OutputString(SystemTable -> ConOut, L"HelloWorld\n"); 
   return EFI_SUCCESS;
}
