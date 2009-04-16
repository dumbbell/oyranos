/** @file oyranos_helper.c
 *
 *  Oyranos is an open source Colour Management System 
 *
 *  @par Copyright:
 *            2005-2009 (C) Kai-Uwe Behrmann
 *
 *  @brief    helpers
 *  @internal
 *  @author   Kai-Uwe Behrmann <ku.b@gmx.de>
 *  @par License:
 *            new BSD <http://www.opensource.org/licenses/bsd-license.php>
 *  @since    2005/09/02
 */


#include "oyranos.h"
#include "oyranos_helper.h"
#include "oyranos_internal.h"

#include "md5.h"
#include "lookup3.h" /* oy_hashlittle */


#include <stdio.h>

intptr_t oy_observe_pointer_ = 0;

/* --- internal API definition --- */
static int oy_alloc_count_ = 0;
static int oy_allocs_count_ = 0;
int oy_debug_memory = 0;

/* internal memory handling */
void* oyAllocateFunc_           (size_t        size)
{
  /* we have most often to work with text arrays, so initialise with 0 */
  void *ptr = calloc (sizeof (char), size);

  if( !ptr )
  {
    WARNc1_S( "Can not allocate %d byte.", (int)size );
  }
    else if(oy_debug_memory)
  {
    oy_alloc_count_ += size;
    printf( "%s:%d %d allocate %d  %d\n", __FILE__,__LINE__,oy_allocs_count_, (int)size, oy_alloc_count_ );
    ++oy_allocs_count_;
  }

  return ptr;
}

void  oyDeAllocateFunc_           (void*       block)
{
  if( !block ) {
    WARNc_S( "Memory block is empty." )
  } else
  {
    free( block );
    if(oy_debug_memory)
      printf( "%s:%d %d deallocated\n", __FILE__,__LINE__,--oy_allocs_count_ );
  }
}

void* oyAllocateWrapFunc_       (size_t        size,
                                 oyAlloc_f     allocate_func)
{
  if(allocate_func)
    return allocate_func (size);
  else
    return oyAllocateFunc_ (size);
}


/** @internal
 *  @brief md5 calculation
 *
 *  @since Oyranos: version 0.1.8
 *  @date  24 november 2007 (API 0.1.8)
 */
int                oyMiscBlobGetMD5_ ( void              * buffer,
                                       size_t              size,
                                       unsigned char     * digest )
{
  DBG_PROG_START

  if (digest) 
  {
    oy_md5_state_t state;

    oy_md5_init(   &state );
    oy_md5_append( &state, (const md5_byte_t *)buffer, size );
    oy_md5_finish( &state, digest );

    DBG_PROG_ENDE
    return 0;

  } else {
    WARNc3_S ("False memory - size = %d pos = %lu digest = %lu",
              (int)size, (long int)buffer, (long int)digest);

    DBG_PROG_ENDE
    return 1;
  }
}

uint32_t           oyMiscBlobGetL3_ ( void              * buffer,
                                       size_t              size )
{
  uint32_t ret = 0;

  ret = oy_hashlittle( buffer, size, 0 );

  return ret;
}


