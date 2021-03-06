/** @file oyranos_sentinel.c
 *
 *  Oyranos is an open source Colour Management System 
 *
 *  @par Copyright:
 *            2006-2011 (C) Kai-Uwe Behrmann
 *
 *  @brief    library sentinels
 *  @internal
 *  @author   Kai-Uwe Behrmann <ku.b@gmx.de>
 *  @par License:
 *            new BSD <http://www.opensource.org/licenses/bsd-license.php>
 *  @since    2006/09/14
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "oyranos.h"
#include "oyranos_alpha_internal.h"
#include "oyranos_debug.h"
#include "oyranos_elektra.h"
#include "oyranos_helper.h"
#include "oyranos_i18n.h"
#include "oyranos_internal.h"
#include "oyranos_sentinel.h"

int oy_sentinel_initialised_ = 0;

void oyInit_()
{
  DBG_PROG_START

  if(oy_sentinel_initialised_)
  {
    DBG_PROG_ENDE
    return;
  }
  oy_sentinel_initialised_ = 1;

  if(getenv("OYRANOS_DEBUG"))
    oy_debug = atoi(getenv("OYRANOS_DEBUG"));

  if(getenv("OYRANOS_DEBUG_MEMORY"))
    oy_debug_memory = atoi(getenv("OYRANOS_DEBUG_MEMORY"));

  if(getenv("OYRANOS_DEBUG_SIGNALS"))
    oy_debug_signals = atoi(getenv("OYRANOS_DEBUG_SIGNALS"));

  if(getenv("OYRANOS_BACKTRACE"))
  {
    oy_backtrace = getenv("OYRANOS_BACKTRACE");
    if(!oy_debug)
      ++oy_debug;
  }

  oyI18NInit_ ();
  DBG_PROG_ENDE
}

int oyExportEnd_()
{
  static int start = 1;
  if(start == 1)
  {
    start = 0;
    return 1;
  }
  return start;
}

void  oyExportResetSentinel_(void)
{
  oy_sentinel_initialised_ = 0;
}


