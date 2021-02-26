/*Main.c */
#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
int
EFIAPI
main (
  IN int Argc,
  IN char **Argv
  )
{
 gST->ConOut->OutputString(gST->ConOut, L"HelloWorld\n"); 
  return 0;
}
