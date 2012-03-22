#include "oyBlob_s.h"
#include "oyOption_s.h"

int main( int argc, char ** argv)
{
  oyBlob_s * blob = oyBlob_New(0);
  oyOption_s * o;

  if(!blob)
    oyMessageFunc_p(oyMSG_WARN,0,"No oyBlob_s generated");

  oyBlob_Release(&blob);

  o = oyOption_FromRegistration("/org/freedesktop.openicc/imaging/key",0);
  oyOption_SetFromDouble( o, 0.25, 2, 0 );
  oyOption_Release( &o );

  return 0;
}

