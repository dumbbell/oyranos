#include <cups/cups.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
  httpInitialize();
  printf("CUPS v%d.%d%d", CUPS_VERSION_MAJOR, CUPS_VERSION_MINOR, CUPS_VERSION_PATCH );
  return 0;
}
