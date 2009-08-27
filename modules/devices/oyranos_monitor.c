/** @file oyranos_monitor.c
 *
 *  Oyranos is an open source Colour Management System 
 *
 *  @par Copyright:
 *            2005-2009 (C) Kai-Uwe Behrmann
 *
 *  @brief    monitor device detection
 *  @internal
 *  @author   Kai-Uwe Behrmann <ku.b@gmx.de>
 *  @par License:
 *            new BSD <http://www.opensource.org/licenses/bsd-license.php>
 *  @since    2005/01/31
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "limits.h"
#include <unistd.h>  /* intptr_t */

#include "config.h"

#if HAVE_X
# include <X11/Xlib.h>
# include <X11/Xutil.h>
# include <X11/Xatom.h>
# if HAVE_XIN
#  include <X11/extensions/Xinerama.h>
# endif
# ifdef HAVE_XF86VMODE
#  include <X11/extensions/xf86vmode.h>
# endif
#endif
#ifdef __APPLE__
/* old style Quicktime APIs */
#include <Carbon/Carbon.h>
/* newer style CoreGraphics APIs like CGDirectDisplay.h and so on */
#include <ApplicationServices/ApplicationServices.h>
#endif

#include "oyranos.h"
#include "oyranos_alpha.h"
#include "oyranos_internal.h"
#include "oyranos_monitor.h"
#include "oyranos_monitor_internal.h"
#include "oyranos_debug.h"
#include "oyranos_helper.h"
#include "oyranos_sentinel.h"

#define DEBUG 1

/* ---  Helpers  --- */

/* --- internal API definition --- */



#if (defined(HAVE_X) && !defined(__APPLE__))
int   oyMonitor_getScreenFromDisplayName_( oyMonitor_s   * disp );
#endif
char**oyGetAllScreenNames_        (const char *display_name, int *n_scr );
int   oyMonitor_getScreenGeometry_   ( oyMonitor_s       * disp );
/** @internal Display functions */
const char* oyMonitor_name_( oyMonitor_s *disp ) { return disp->name; }
const char* oyMonitor_hostName_( oyMonitor_s *disp ) { return disp->host; }
const char* oyMonitor_identifier_( oyMonitor_s *disp ) { return disp->identifier; }
/** @internal the screen appendment for the root window properties */
char*       oyMonitor_screenIdentifier_( oyMonitor_s *disp )
{ char *number = 0;

  oyAllocHelper_m_( number, char, 24, 0, return "");
  number[0] = 0;
  if( disp->geo[1] >= 1 && !disp->screen ) sprintf( number,"_%d", disp->geo[1]);
  return number;
}
int oyMonitor_deviceScreen_( oyMonitor_s *disp ) { return disp->screen; }
int oyMonitor_number_( oyMonitor_s *disp ) { return disp->geo[0]; }
int oyMonitor_screen_( oyMonitor_s *disp ) { return disp->geo[1]; }
int oyMonitor_x_( oyMonitor_s *disp ) { return disp->geo[2]; }
int oyMonitor_y_( oyMonitor_s *disp ) { return disp->geo[3]; }
int oyMonitor_width_( oyMonitor_s *disp ) { return disp->geo[4]; }
int oyMonitor_height_( oyMonitor_s *disp ) { return disp->geo[5]; }
int   oyMonitor_getGeometryIdentifier_(oyMonitor_s       * disp );
#if defined(__APPLE__)
CGDirectDisplayID oyMonitor_device_( oyMonitor_s *disp ) { return disp->id; }
#elif defined(HAVE_X)
Display* oyMonitor_device_( oyMonitor_s *disp ) { return disp->display; }
const char* oyMonitor_systemPort_( oyMonitor_s *disp ) { return disp->system_port; }
oyBlob_s *  oyMonitor_edid_( oyMonitor_s * disp ) { return oyBlob_Copy( disp->edid, 0 ); }

oyX11INFO_SOURCE_e
    oyMonitor_infoSource_( oyMonitor_s *disp ) { return disp->info_source; }
# ifdef HAVE_XRANDR
XRRScreenResources *
    oyMonitor_xrrResource_( oyMonitor_s * disp ) { return disp->res; }
RROutput
    oyMonitor_xrrOutput_( oyMonitor_s * disp ) { return disp->output; }
XRROutputInfo *
    oyMonitor_xrrOutputInfo_( oyMonitor_s * disp ) { return disp->output_info; }
int oyMonitor_activeOutputs_( oyMonitor_s * disp ) { return disp->active_outputs; }
#endif

char* oyMonitor_getAtomName_         ( oyMonitor_s       * disp,
                                       const char        * base );
char* oyChangeScreenName_            ( const char        * display_name,
                                       int                 screen );


const char *xrandr_edids[] = {"EDID","EDID_DATA",0};

int
oyFree_( void *oy_structure )
{ int error = 0;
  oyOBJECT_e *type = (oyOBJECT_e*) oy_structure;

  if( type )
    switch( *type )
    {
    case oyOBJECT_MONITOR_S:
      {
        oyMonitor_s *disp = (oyMonitor_s*)oy_structure;

        oyFree_m_( disp->name )
        oyFree_m_( disp->host )
        oyFree_m_( disp->identifier )
        if(disp->system_port)
          oyFree_m_( disp->system_port );
        oyBlob_Release( &disp->edid );
        disp->geo[0] = disp->geo[1] = -1;
        if( disp->display ) { XCloseDisplay( disp->display ); disp->display=0;}
      }
      break;
    default: WARNc_S ("unknown type"); error = 1;
    }
  else
    error = 1;

  return error;
}
#endif

/** @internal oyUnrollEdid1_ */
void
oyUnrollEdid1_                    (struct oyDDC_EDID1_s_ *edi,
                                   char**      manufacturer,
                                   char**      model,
                                   char**      serial,
                                   oyAlloc_f     allocate_func)
{
  char *t = 0;
  int len, i;
  char mnf[4];

  DBG_PROG_START

  /* check */
  if(edi &&
     edi->sig[0] == 0 &&
     edi->sig[1] == 255 &&
     edi->sig[2] == 255 &&
     edi->sig[3] == 255 &&
     edi->sig[4] == 255 &&
     edi->sig[5] == 255 &&
     edi->sig[6] == 255 &&
     edi->sig[7] == 0 
    ) {
    /* verified */
  } else {
    WARNc_S("Could not verifiy EDID")
    DBG_PROG_ENDE
    return;
  }

  sprintf( mnf, "%c%c%c",
          (char)((edi->MNF_ID[0] & 124) >> 2) + 'A' - 1,
          (char)((edi->MNF_ID[0] & 3) << 3) + ((edi->MNF_ID[1] & 227) >> 5) + 'A' - 1,
          (char)(edi->MNF_ID[1] & 31) + 'A' - 1 );

  /*printf( "MNF_ID: %d %d SER_ID: %d %d D:%d/%d bxh:%dx%dcm %s\n",
           edi->MNF_ID[0], edi->MNF_ID[1], edi->SER_ID[0], edi->SER_ID[1],
           edi->WEEK, edi->YEAR +1990,
           edi->width, edi->height, mnf );*/


  for( i = 0; i < 4; ++i)
  {
    unsigned char *block = edi->text1 + i * 18;
    char **target = NULL,
         * tmp = 0;

    if(block[0] == 0 && block[1] == 0 && block[2] == 0)
    {
      if( block[3] == 255 ) { /* serial */
        target = serial;
      } else if( block[3] == 254 ) { /* vendor */
        target = manufacturer;
      } else if( block[3] == 253 ) { /* frequenz ranges */
      } else if( block[3] == 252 ) { /* model */
        target = model;
      }
      if(target)
      {
        len = strlen((char*)&block[5]); DBG_PROG_V((len))
        if(len) { DBG_PROG
          ++len;
          t = (char*)oyAllocateWrapFunc_( 16, allocate_func );
          oySnprintf_(t, 15, (char*)&block[5]);
          t[15] = '\000';
          if(oyStrrchr_(t, '\n'))
          {
            tmp = oyStrrchr_(t, '\n');
            tmp[0] = 0;
          }
          *target = t; DBG_PROG_S( *target )
        }
      }
    } 
  }

  if(!*manufacturer)
  {
    *manufacturer = (char*)oyAllocateWrapFunc_( 24, allocate_func );
    if(!strcmp(mnf,"APP"))
      sprintf(*manufacturer, "Apple");
    else if(!strcmp(mnf,"PHL"))
      sprintf(*manufacturer, "Philips");
    else if(!strcmp(mnf,"NEC"))
      sprintf(*manufacturer, "NEC");
    else if(!strcmp(mnf,"EIZ"))
      sprintf(*manufacturer, "EIZO");
    else if(!strcmp(mnf,"MEI"))
      sprintf(*manufacturer, "Panasonic");
    else if(!strcmp(mnf,"MIR"))
      sprintf(*manufacturer, "miro");
    else if(!strcmp(mnf,"SNI"))
      sprintf(*manufacturer, "Siemens Nixdorf");
    else if(!strcmp(mnf,"SNY"))
      sprintf(*manufacturer, "Sony");
    else
      sprintf(*manufacturer, mnf);
  }

  DBG_PROG_ENDE
}



#if defined( HAVE_X ) && !defined(__APPLE__)
/** @internal
 *  Function oyMonitor_getProperty_
 *  @brief   obtain X property
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/01/17 (Oyranos: 0.1.10)
 *  @date    2009/08/18
 */
oyBlob_s *   oyMonitor_getProperty_  ( oyMonitor_s       * disp,
                                       const char        * prop_name,
                                       const char       ** prop_name_xrandr )
{
  oyBlob_s * prop = 0;
  Display *display = 0;
  Window w = 0;
  Atom atom = 0, a;
  char *atom_name;
  int actual_format_return;
  unsigned long nitems_return=0, bytes_after_return=0;
  unsigned char* prop_return=0;
  int error = !disp;

  if(!error)
  {
    display = oyMonitor_device_( disp );
# ifdef HAVE_XRANDR
    if( oyMonitor_infoSource_( disp ) == oyX11INFO_SOURCE_XRANDR )
    {
      int pos = 0, i;
      if(prop_name_xrandr)
        while(!atom && prop_name_xrandr[i])
          atom = XInternAtom( display,
                              prop_name_xrandr[i++],
                              True );
      else
        atom = XInternAtom( display, prop_name, True );
      DBG_PROG1_S("atom: %ld", atom)

      if(atom)
      {
        error =
        XRRGetOutputProperty ( display, oyMonitor_xrrOutput_( disp ),
                      atom, 0, 32,
                      False, False, AnyPropertyType, &a,
                      &actual_format_return, &nitems_return,
                      &bytes_after_return, &prop_return );

      }
    }
# endif
    atom = 0;

    if( oyMonitor_infoSource_( disp ) == oyX11INFO_SOURCE_XINERAMA ||
        (oyMonitor_infoSource_( disp ) == oyX11INFO_SOURCE_XRANDR &&
          !nitems_return) )
    {
      atom_name = oyMonitor_getAtomName_( disp, prop_name );
      if(atom_name)
        atom = XInternAtom(display, atom_name, True);
      if(atom)
        w = RootWindow( display, oyMonitor_deviceScreen_( disp ) );
      if(w)
        /* AnyPropertyType does not work for _ICC_PROFILE ---vvvvvvvvvv */
        XGetWindowProperty( display, w, atom, 0, INT_MAX, False, XA_CARDINAL,
                     &a, &actual_format_return, &nitems_return, 
                     &bytes_after_return, &prop_return );
      if(atom_name)
        oyFree_m_( atom_name )
    }
  }

  if(nitems_return && prop_return)
  {
    prop = oyBlob_New( 0 );
    oyBlob_SetFromData( prop, prop_return, nitems_return + bytes_after_return, 0 );
    XFree( prop_return ); prop_return = 0;
  }

  return prop;
}

int
oyGetMonitorInfo_                 (const char* display_name,
                                   char**      manufacturer,
                                   char**      model,
                                   char**      serial,
                                   char**      display_geometry,
                                       char             ** system_port,
                                       char             ** host,
                                       oyBlob_s         ** edid,
                                   oyAlloc_f     allocate_func,
                                       oyStruct_s        * user_data )
{
  int len;
  struct oyDDC_EDID1_s_ *edi=0;
  char *t;
  oyMonitor_s * disp = 0;
  oyBlob_s * prop = 0;

  DBG_PROG_START

  if(display_name)
    DBG_PROG1_S("display_name %s",display_name);

  disp = oyMonitor_newFrom_( display_name, 1 );
  if(!disp)
    return 1;

  if(!allocate_func)
    allocate_func = oyAllocateFunc_;

  if( system_port ) 
  {
    t = 0;
    if( oyMonitor_systemPort_( disp ) &&
        oyStrlen_(oyMonitor_systemPort_( disp )) )
    {
      len = oyStrlen_(oyMonitor_systemPort_( disp ));
      ++len;
      t = (char*)oyAllocateWrapFunc_( len, allocate_func );
      sprintf(t, oyMonitor_systemPort_( disp ));
    }
    *system_port = t; t = 0;
  }

  if( display_geometry )
    *display_geometry = oyStringCopy_( oyMonitor_identifier_( disp ),
                                       allocate_func );

  if( host )
    *host = oyStringCopy_( oyMonitor_hostName_( disp ), allocate_func );

  prop = oyMonitor_getProperty_( disp, "XFree86_DDC_EDID1_RAWDATA",
                                       xrandr_edids );

  if( oyMonitor_infoSource_( disp ) == oyX11INFO_SOURCE_XINERAMA &&
      (!prop || (prop && prop->size != 128)) )
  {
    int error = 0;
    char * txt = oyAllocateFunc_(1024); txt[0] = 0;

    /* test twinview edid */
    if(oy_debug)
      oySnprintf1_( txt, 1024, "OYRANOS_DEBUG=%d ", oy_debug);
    oySnprintf1_( &txt[strlen(txt)], 1024, "%s",
              "PATH=" OY_BINDIR ":$PATH; oyranos-monitor-nvidia -p" );

    error = system( txt );
    if(txt) { oyDeAllocateFunc_(txt); txt = 0; }

    prop = oyMonitor_getProperty_( disp, "XFree86_DDC_EDID1_RAWDATA",
                                         xrandr_edids );
  }

  if( prop )
  {
    if( prop->size != 128 )
    {
      WARNcc4_S(user_data, "\n\t  %s %d; %s %s",_("unexpected EDID lenght"),
               (int)prop->size,
               "\"XFree86_DDC_EDID1_RAWDATA\"/\"EDID_DATA\"",
               _("Cant read hardware information from device."))
    } else
    {
      /* convert to an deployable struct */
      edi = (struct oyDDC_EDID1_s_*) prop->ptr;

      oyUnrollEdid1_( edi, manufacturer, model, serial, allocate_func );
    }
  }

  if(edid)
  {
    *edid = prop;
    prop = 0;
  }

  oyMonitor_release_( &disp );

  if(prop || (edid && *edid))
  {
    oyBlob_Release( &prop );
    DBG_PROG_ENDE
    return 0;
  } else {
    WARNcc3_S( user_data, "\n  %s:\n  %s\n  %s",
               _("no EDID available from X properties"),
               "\"XFree86_DDC_EDID1_RAWDATA\"/\"EDID_DATA\"",
               _("Cant read hardware information from device."))
    DBG_PROG_ENDE
    return -1;
  }
}
#endif

#ifdef __APPLE__
OSErr oyCMIterateDeviceInfoProc          ( const CMDeviceInfo* dev_info,
                                           void              * ptr )
{
  char ** my_data = (char**) ptr;
  char dev_class[8] = {0,0,0,0,0,0,0,0};

  memcpy( dev_class, &dev_info->deviceClass, 4 );
  dev_class[4] = 0;

  /* watch the device */
  /*WARNc2_S( "%s %d", dev_class, dev_info->deviceID );*/

  if(dev_info->deviceClass == cmDisplayDeviceClass)
  {
  }

  return noErr;
}

CGDirectDisplayID oyMonitor_nameToOsxID ( const char        * display_name )
{
#if 0
  DisplayIDType screenID=0;
  GDHandle device = 0;

  if(!display_name || 
     atoi(display_name) <= 0)
  {
    device = DMGetFirstScreenDevice(true); /*GetDeviceList();*/
    DMGetDisplayIDByGDevice(device, &screenID, false);
  } else
    screenID = atoi(display_name);

  return screenID;
#endif

  int pos = 0;
  CGDisplayErr err = kCGErrorSuccess;
  CGDisplayCount alloc_disps = 0;
  CGDirectDisplayID * active_displays = 0,
                    cg_direct_display_id = 0;
  CGDisplayCount count = 0;
  io_service_t io_service = 0;

  err = CGGetActiveDisplayList( alloc_disps, active_displays, &count );
  if(count <= 0 || err != kCGErrorSuccess)
  {
    WARNc2_S("%s %s", _("open X Display failed"), display_name)
    return 0;
  }
  alloc_disps = count + 1;
  oyAllocHelper_m_( active_displays, CGDirectDisplayID, alloc_disps,0,return 0);
  if(active_displays)
    err = CGGetActiveDisplayList( alloc_disps, active_displays, &count);

  if(display_name && display_name[0])
    pos = atoi( display_name );

  if(err)
  {
    WARNc1_S( "CGGetActiveDisplayList call with error %d", err );
  } else
  {
    cg_direct_display_id = active_displays[pos];

    oyFree_m_( active_displays );
  }

  return cg_direct_display_id;
}
#endif



char *       oyX1GetMonitorProfile   ( const char        * device_name,
                                       size_t            * size,
                                       oyAlloc_f           allocate_func )
{
  char       *moni_profile=0;
  int error = 0;

#ifdef __APPLE__

  CMProfileRef prof = NULL;
  CGDirectDisplayID screenID = 0;
  CMProfileLocation loc;
  int err = 0;
  char * block = NULL;

  DBG_PROG_START

  screenID = oyMonitor_nameToOsxID( device_name );

  CMGetProfileByAVID( (CMDisplayIDType)screenID, &prof );
  CMGetProfileLocation(prof, &loc);

  err = oyGetProfileBlockOSX (prof, &block, size, allocate_func);
  moni_profile = block;
  if (prof) CMCloseProfile(prof);

  {
    UInt32 seed = 0,
           count = 0;
    char * my_data = 0;
    CMError cm_err = CMIterateColorDevices( oyCMIterateDeviceInfoProc, 
                                            &seed, &count, &my_data );
  }

#else /* HAVE_X */

  oyMonitor_s * disp = 0;
  oyBlob_s * prop = 0;

  DBG_PROG_START

  if(device_name)
    DBG_PROG1_S("device_name %s",device_name);

  disp = oyMonitor_newFrom_( device_name, 0 );
  if(!disp)
    return 0;

  prop = oyMonitor_getProperty_( disp, "_ICC_PROFILE", 0 );

  if(prop)
  {
    oyAllocHelper_m_( moni_profile, char, prop->size, allocate_func, error = 1 )
    if(!error)
      error = !memcpy( moni_profile, prop->ptr, prop->size );
    if(!error)
      *size = prop->size;
    oyBlob_Release( &prop );
  } /*else
    WARNc1_S("\n  %s",
         _("Could not get Xatom, probably your monitor profile is not set:"));*/

  oyMonitor_release_( &disp );
#endif /* TODO WIN */

  DBG_PROG_ENDE
  if(!error)
    return moni_profile;
  else
    return NULL;
}


int      oyGetAllScreenNames         ( const char        * display_name,
                                       char            *** display_names,
                                       oyAlloc_f           allocateFunc )
{
  int i = 0;
  char** list = 0;

  list = oyGetAllScreenNames_( display_name, &i );

  *display_names = 0;

  if(list && i)
  {
    *display_names = oyStringListAppend_( 0, 0, (const char**)list, i, &i,
                                          allocateFunc );
    oyStringListRelease_( &list, i, oyDeAllocateFunc_ );
  }

  return i; 
}

char**
oyGetAllScreenNames_            (const char *display_name,
                                 int *n_scr)
{
  int i = 0;
  char** list = 0;

#ifdef __APPLE__

  int ids[256];
  CGDisplayErr err = kCGErrorSuccess;
  CGDisplayCount alloc_disps = 0;
  CGDirectDisplayID * active_displays = 0;
  CGDisplayCount count = 0;
  io_service_t io_service = 0;

  *n_scr = 0;

  err = CGGetActiveDisplayList( alloc_disps, active_displays, &count );
  if(count <= 0 || err != kCGErrorSuccess)
  {
    WARNc2_S("%s %s", _("open X Display failed"), display_name)
    return 0;
  }
  alloc_disps = count + 1;
  oyAllocHelper_m_( active_displays, CGDirectDisplayID, alloc_disps,0,return 0);
  if(active_displays)
    err = CGGetActiveDisplayList( alloc_disps, active_displays, &count);
  for(i = 0; i < count && i < 256; ++i)
  {
    io_service = CGDisplayIOServicePort ( active_displays[i] );
    ids[i] = i;
  }
  i = count;

  *n_scr = i;
  oyAllocHelper_m_( list, char*, *n_scr, 0, return NULL )

  for ( i = 0; i < *n_scr ; ++i )
  {
    oyAllocHelper_m_( list[i], char, 24, 0, return NULL )
    oySnprintf1_( list[i], 24, "%d", ids[i] );
  }

#else /* HAVE_X */
  Display *display = 0;
  int len = 0;
  oyMonitor_s * disp = 0;

  *n_scr = 0;

  if(!display_name || !display_name[0])
    return 0;

  disp = oyMonitor_newFrom_( display_name, 0 );
  if(!disp)
    return 0;

  display = oyMonitor_device_( disp );

  if( !display || (len = ScreenCount( display )) == 0 )
    return 0;

# if HAVE_XRANDR
  if( oyMonitor_infoSource_( disp ) == oyX11INFO_SOURCE_XRANDR)
    len = disp->active_outputs;
# endif

# if HAVE_XIN
  /* test for Xinerama screens */
  if( oyMonitor_infoSource_( disp ) == oyX11INFO_SOURCE_XINERAMA)
    {
      int n_scr_info = 0;
      XineramaScreenInfo *scr_info = XineramaQueryScreens( display, &n_scr_info );
      oyPostAllocHelper_m_(scr_info, n_scr_info, return 0 )

      if( n_scr_info >= 1 )
        len = n_scr_info;

      XFree( scr_info );
    }
# endif

  oyAllocHelper_m_( list, char*, len, 0, return NULL )

  for (i = 0; i < len; ++i)
    if( (list[i] = oyChangeScreenName_( display_name, i )) == 0 )
      return NULL;

  *n_scr = len;
  oyMonitor_release_( &disp );

#endif

  return list;
}


/** @internal
 *  Function oyX1Rectangle_FromDevice
 *  @memberof monitor_api
 *  @brief   value filled a oyStruct_s object
 *
 *  @param         device_name         the Oyranos specific device name
 *  @return                            the rectangle the device projects to
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/01/28 (Oyranos: 0.1.10)
 *  @date    2009/01/28
 */
oyRectangle_s* oyX1Rectangle_FromDevice ( const char        * device_name )
{
  oyRectangle_s * rectangle = 0;
  int error = !device_name;

  if(!error)
  {
    oyMonitor_s * disp = 0;

    disp = oyMonitor_newFrom_( device_name, 0 );
    if(!disp)
      return 0;

    rectangle = oyRectangle_NewWith( oyMonitor_x_(disp), oyMonitor_y_(disp),
                           oyMonitor_width_(disp), oyMonitor_height_(disp), 0 );

    oyMonitor_release_( &disp );
  }

  return rectangle;
}


/** @internal
    takes a chaked display name and point as argument and
    gives a string back for search in the db
 */
int
oyMonitor_getGeometryIdentifier_         (oyMonitor_s  *disp)
{
  int len = 64;

  oyAllocHelper_m_( disp->identifier, char, len, 0, return 1 )

  oySnprintf4_( disp->identifier, len, "%dx%d+%d+%d", 
            oyMonitor_width_(disp), oyMonitor_height_(disp),
            oyMonitor_x_(disp), oyMonitor_y_(disp) );

  return 0;
}

#if defined( HAVE_X ) && !defined(__APPLE)
char* oyMonitor_getAtomName_         ( oyMonitor_s       * disp,
                                       const char        * base )
{
  int len = 64;
  char *atom_name = 0;
  char *screen_number = oyMonitor_screenIdentifier_( disp );

  oyPostAllocHelper_m_( screen_number, 1, return 0 )
  oyAllocHelper_m_( atom_name, char, len, 0, return 0 )

  oySnprintf2_( atom_name, len, "%s%s", base, screen_number );

  oyFree_m_( screen_number );

  return atom_name;
}
#endif


int      oyX1MonitorProfileSetup     ( const char        * display_name,
                                       const char        * profil_name )
{
  int error = 0;
  const char * profile_fullname = 0;
  const char * profil_basename = 0;
  char* profile_name_ = 0;
  oyProfile_s * prof = 0;
#if defined( HAVE_X ) && !defined(__APPLE__)
  oyMonitor_s * disp = 0;
  char       *dpy_name = NULL;
  char *text = 0;
#endif

  DBG_PROG_START
#if defined( HAVE_X ) && !defined(__APPLE__)
  disp = oyMonitor_newFrom_( display_name, 0 );
  if(!disp)
    return 1;

  dpy_name = calloc( sizeof(char), MAX_PATH );
  if( display_name && !strstr( disp->host, display_name ) )
    oySnprintf1_( dpy_name, MAX_PATH, ":%d", disp->geo[0] );
  else
    oySnprintf2_( dpy_name, MAX_PATH, "%s:%d", disp->host, disp->geo[0] );
#endif

  if(profil_name)
  {
    DBG_PROG1_S( "profil_name = %s", profil_name );
    prof = oyProfile_FromFile( profil_name, 0, 0 );
    profile_fullname = oyProfile_GetFileName( prof, -1 );
  }

  if( profile_fullname && profile_fullname[0] )
  {

    if(profil_name && strrchr(profil_name,OY_SLASH_C))
      profil_basename = strrchr(profil_name,OY_SLASH_C)+1;
    else
    {
      if(profil_name)
        profil_basename = profil_name;
      else if( profile_fullname && strrchr(profile_fullname,OY_SLASH_C))
        profil_basename = strrchr( profile_fullname, OY_SLASH_C)+1;
    }

#ifdef __APPLE__
    {
      CMProfileLocation loc;
      CMError err = 0;
      CMProfileRef prof=NULL;
      CGDirectDisplayID screenID = 0;

      loc.locType = cmPathBasedProfile;
      oySnprintf1_( loc.u.pathLoc.path, 255, "%s", profile_fullname);

      err = CMOpenProfile ( &prof, &loc );
      screenID = oyMonitor_nameToOsxID( display_name );

      if( screenID && !err )
        err = CMSetProfileByAVID ( (CMDisplayIDType)screenID, prof );

      CMCloseProfile( prof );
    }
#else /* HAVE_X */

    oyAllocHelper_m_( text, char, MAX_PATH, 0, error = 1; goto Clean )
    DBG_PROG1_S( "profile_fullname %s", profile_fullname );

    /** set vcgt tag with xcalib
       not useable with multihead Xinerama at one screen

       @todo TODO xcalib should be configurable as a module
     */
    sprintf(text,"xcalib -d %s -s %d %s \'%s\'", dpy_name, disp->geo[1],
                 oy_debug?"-v":"", profile_fullname);
    {
      Display * display = oyMonitor_device_( disp );
      int effective_screen = oyMonitor_screen_( disp );
      int screen = oyMonitor_deviceScreen_( disp );
      XF86VidModeGamma gamma;
      int size = 0,
          can_gamma = 0;

      if(!display)
      {
        WARNc3_S("%s %s %s", _("open X Display failed"), dpy_name, display_name)
        return 1;
      }

#ifdef HAVE_XF86VMODE
      if(effective_screen == screen)
      {
        /* check for gamma capabiliteis on the given screen */
        if (XF86VidModeGetGamma(display, effective_screen, &gamma))
          can_gamma = 1;
        else
        if (XF86VidModeGetGammaRampSize(display, effective_screen, &size))
        {
          if(size)
            can_gamma = 1;
        }
      }
#endif

      /* Check for incapabilities of X gamma table access */
      if(can_gamma || oyMonitor_screen_( disp ) == 0)
        error = system(text);
      if(error &&
         error != 65280)
      { /* hack */
        WARNc2_S("%s %s", _("No monitor gamma curves by profile:"),
                oyNoEmptyName_m_(profil_basename) )
      } else
        /* take xcalib error not serious, turn into a issue */
        error = -1;
    }

    DBG_PROG1_S( "system: %s", text )

    /* set _ICC_PROFILE atom in X */
    {
      Display *display;
      Atom atom = 0;
      int screen = 0;
      Window w;

      unsigned char *moni_profile=0;
      size_t      size=0;
      char       *atom_name=0;
      int         result = 0;

      if(display_name)
        DBG_PROG1_S("display_name %s",display_name);

      display = oyMonitor_device_( disp );

      screen = oyMonitor_deviceScreen_( disp );
      DBG_PROG_V((screen))
      w = RootWindow(display, screen); DBG_PROG1_S("w: %ld", w)

      if(profile_fullname)
        moni_profile = oyGetProfileBlock( profile_fullname, &size, oyAllocateFunc_ );
      else if(profil_name)
        moni_profile = oyGetProfileBlock( profil_name, &size, oyAllocateFunc_ );

      if(!size || !moni_profile)
        WARNc_S(_("Error obtaining profile"));

      atom_name = oyMonitor_getAtomName_( disp, "_ICC_PROFILE" );
      if( atom_name )
      {
        atom = XInternAtom (display, atom_name, False);
        if (atom == None) {
          WARNc2_S("%s \"%s\"", _("Error setting up atom"), atom_name);
        }
      } else WARNc_S(_("Error setting up atom"));

      if( atom && moni_profile)
      result = XChangeProperty( display, w, atom, XA_CARDINAL,
                       8, PropModeReplace, moni_profile, (int)size );

      if(moni_profile)
        oyFree_m_( moni_profile )
      oyFree_m_( atom_name )
    }

    oyFree_m_( text );
#endif
  }

#if defined( HAVE_X ) && !defined(__APPLE__)
  Clean:
  oyMonitor_release_( &disp );
#endif
  oyProfile_Release( &prof );
  if(profile_name_) oyFree_m_( profile_name_ );
#if defined( HAVE_X ) && !defined(__APPLE__)
  if(dpy_name) oyFree_m_( dpy_name );
#endif

  DBG_PROG_ENDE
  return error;
}


int      oyX1MonitorProfileUnset     ( const char        * display_name )
{
  int error = 0;

#ifdef __APPLE__

    {
      CMError err = 0;
      CMProfileRef prof=NULL;
      CGDirectDisplayID screenID = 0;


      screenID = oyMonitor_nameToOsxID( display_name );

      if( screenID && !err )
        err = CMSetProfileByAVID ( (CMDisplayIDType)screenID, prof );

    }

#else /* HAVE_X */

  oyMonitor_s * disp = 0;
  oyProfile_s * prof = 0;

  DBG_PROG_START

  disp = oyMonitor_newFrom_( display_name, 0 );
  if(!disp)
  {
    DBG_PROG_ENDE
    return 1;
  }


  {
    /* unset the _ICC_PROFILE atom in X */
      Display *display;
      Atom atom;
      int screen = 0;
      Window w;
      char *atom_name = 0;

      if(display_name)
        DBG_PROG1_S("display_name %s",display_name);

      display = oyMonitor_device_( disp );

      screen = oyMonitor_deviceScreen_( disp );
      DBG_PROG_V((screen))
      w = RootWindow(display, screen); DBG_PROG1_S("w: %ld", w)

      DBG_PROG

      atom_name = oyMonitor_getAtomName_( disp, "_ICC_PROFILE" );
      atom = XInternAtom (display, atom_name, True);
      if (atom != None)
        XDeleteProperty( display, w, atom );
      else
      {
        WARNc2_S("%s \"%s\"", _("Error getting atom"), atom_name);
      }

      {
        char *dpy_name = oyStringCopy_( oyNoEmptyString_m_(display_name), oyAllocateFunc_ );
        char * command = 0;
        char *ptr = NULL;
        int r;

        oyAllocHelper_m_( command, char, 1048, 0 , goto finish );

        if( (ptr = strchr(dpy_name,':')) != 0 )
          if( (ptr = strchr(ptr,'.')) != 0 )
            ptr[0] = '\000';

        oySnprintf2_(command, 1024, "xgamma -gamma 1.0 -screen %d -display %s",
                 disp->geo[1], dpy_name);

        if(screen == disp->geo[1])
          r = system( command );

        oyFree_m_( command )
      }

      oyFree_m_( atom_name )
      DBG_PROG
    goto finish;
  }


  finish:
  oyProfile_Release( &prof );
  oyMonitor_release_( &disp );
#endif

  DBG_PROG_ENDE
  return error;
}

#if defined( HAVE_X ) && !defined(__APPLE)
int
oyGetDisplayNumber_        (oyMonitor_s *disp)
{
  int dpy_nummer = 0;
  const char *display_name = oyMonitor_name_(disp);

  DBG_PROG_START

  if( display_name )
  {
    char ds[8];             /* display.screen*/
    const char *txt = strchr( display_name, ':' );
    
    if( !txt )
    { WARNc1_S( "invalid display name: %s", display_name )
      return -1;
    }

    ++txt;
    oySnprintf_( ds, 8, txt );
    if( strrchr( ds, '.' ) )
    {
      char *end = strchr( ds, '.' );
      if( end )
        *end = 0;
    }
    dpy_nummer = atoi( ds );
  }

  DBG_PROG_ENDE
  return dpy_nummer;
}

int   oyMonitor_getScreenFromDisplayName_( oyMonitor_s   * disp )
{
  int scr_nummer = 0;
  const char *display_name = oyMonitor_name_(disp);

  DBG_PROG_START

  if( display_name )
  {
    char ds[8];             /* display.screen*/
    const char *txt = strchr( display_name, ':' );
    
    if( !txt )
    { WARNc1_S( "invalid display name: %s", display_name )
      return -1;
    }

    oySnprintf_( ds, 8, txt );
    if( strrchr( display_name, '.' ) )
    {
      char *nummer_text = strchr( ds, '.' );
      if( nummer_text )
        scr_nummer = atoi( &nummer_text[1] );
    }
  }

  DBG_PROG_ENDE
  return scr_nummer;
}


/**  @internal
 *  extract the host name or get from environment
 */
char*
oyExtractHostName_           (const char* display_name)
{
  char* host_name = 0;

  DBG_PROG_START

  oyAllocHelper_m_( host_name, char, strlen( display_name ) + 48,0,return NULL);

  /* Is this X server identifyable? */
  if(!display_name)
  {
    char *host = getenv ("HOSTNAME");
    if (host) {
        sprintf( host_name, host );
    }
  } else if (strchr(display_name,':') == display_name ||
             !strchr( display_name, ':' ) )
  {
    char *host = getenv ("HOSTNAME");
    /* good */
    if (host) {
        sprintf( host_name, host );
    }
  } else if ( strchr( display_name, ':' ) )
  {
    char* ptr = 0;
    sprintf( host_name, display_name );
    ptr = strchr( host_name, ':' );
    ptr[0] = 0;
  }

  DBG_PROG1_S( "host_name = %s", host_name )

  DBG_PROG_ENDE
  return host_name;
}

/** @internal Do a full check and change the screen name,
 *  if the screen arg is appropriate. Dont care about the host part
 */
char*
oyChangeScreenName_                (const char* display_name,
                                    int         screen)
{
  char* host_name = 0;

  DBG_PROG_START

  /* Is this X server identifyable? */
  if(!display_name)
    display_name = ":0.0";


  oyAllocHelper_m_( host_name, char, strlen( display_name ) + 48,0,return NULL);

  sprintf( host_name, display_name );

  /* add screen */
  {
    const char *txt = strchr( host_name, ':' );

    /* fail if no display was given */
    if( !txt )
    { WARNc1_S( "invalid display name: %s", display_name )
      host_name[0] = 0;
      return host_name;
    }

    if( !strchr( display_name, '.' ) )
    {
      sprintf( &host_name[ strlen(host_name) ], ".%d", screen );
    } else
    if( screen >= 0 )
    {
      char *txt_scr = strchr( txt, '.' );
      sprintf( txt_scr, ".%d", screen );
    }
  }

  DBG_PROG1_S( "host_name = %s", host_name )

  DBG_PROG_ENDE
  return host_name;
}

/** @internal get the geometry of a screen 
 *
 *  @param          disp      display info structure
 *  @return         error
 */
int
oyMonitor_getScreenGeometry_            (oyMonitor_s *disp)
{
  int error = 0;
  int screen = 0;


  disp->geo[0] = oyGetDisplayNumber_( disp );
  disp->geo[1] = screen = oyMonitor_getScreenFromDisplayName_( disp );

  if(screen < 0)
    return screen;

# if HAVE_XRANDR
  if( oyMonitor_infoSource_( disp ) == oyX11INFO_SOURCE_XRANDR )
  {
    XRRCrtcInfo * crtc_info = 0;

    crtc_info = XRRGetCrtcInfo( disp->display, disp->res,
                                disp->output_info->crtc );
    if(crtc_info)
    {
      disp->geo[2] = crtc_info->x;
      disp->geo[3] = crtc_info->y;
      disp->geo[4] = crtc_info->width;
      disp->geo[5] = crtc_info->height;

      XRRFreeCrtcInfo( crtc_info );
    } else
    {
      WARNc3_S( "%s output: \"%s\" crtc: %d",
               _("XRandR CrtcInfo request failed"), 
               oyNoEmptyString_m_(disp->output_info->name),
               disp->output_info->crtc)
    }
  }
# endif /* HAVE_XRANDR */

# if HAVE_XIN
  if( oyMonitor_infoSource_( disp ) == oyX11INFO_SOURCE_XINERAMA )
  {
    int n_scr_info = 0;

    XineramaScreenInfo *scr_info = XineramaQueryScreens( disp->display,
                                                         &n_scr_info );
    oyPostAllocHelper_m_(scr_info, n_scr_info, return 1 )

    if( !scr_info )
    {
      WARNc_S(_("Xinerama request failed"))
      return 1;
    }
    {
        disp->geo[2] = scr_info[screen].x_org;
        disp->geo[3] = scr_info[screen].y_org;
        disp->geo[4] = scr_info[screen].width;
        disp->geo[5] = scr_info[screen].height;
    }
    XFree( scr_info );
  }
# endif /* HAVE_XIN */

  if( oyMonitor_infoSource_( disp ) == oyX11INFO_SOURCE_SCREEN )
  {
    Screen *scr = XScreenOfDisplay( disp->display, screen );
    oyPostAllocHelper_m_(scr, 1, WARNc_S(_("open X Screen failed")); return 1;)
    {
        disp->geo[2] = 0;
        disp->geo[3] = 0;
        disp->geo[4] = XWidthOfScreen( scr );
        disp->geo[5] = XHeightOfScreen( scr );
        disp->screen = screen;
    }
  }

  return error;
}

/** @internal
 *  @brief create a monitor information struct for a given display name
 *
 *  @param   display_name              Oyranos display name
 *  @param   expensive                 probe XRandR even if it causes flickering
 *  @return                            a monitor information structure
 *
 *  @version Oyranos: 0.1.10
 *  @since   2007/12/17 (Oyranos: 0.1.8)
 *  @date    2009/01/13
 */
oyMonitor_s* oyMonitor_newFrom_      ( const char        * display_name,
                                       int                 expensive )
{
  int error = 0;
  int i = 0;
  oyMonitor_s * disp = 0;

  DBG_PROG_START

  disp = oyAllocateFunc_( sizeof(oyMonitor_s) );
  error = !disp;
  if(!error)
    error = !memset( disp, 0, sizeof(oyMonitor_s) );

  disp->type_ = oyOBJECT_MONITOR_S;

  if( display_name )
  {
    if( display_name[0] )
      disp->name = oyStringCopy_( display_name, oyAllocateFunc_ );
  } else
  {
    if(getenv("DISPLAY") && strlen(getenv("DISPLAY")))
      disp->name = oyStringCopy_( getenv("DISPLAY"), oyAllocateFunc_ );
    else
      disp->name = oyStringCopy_( ":0", oyAllocateFunc_ );
  }

  if( !error &&
      (disp->host = oyExtractHostName_( disp->name )) == 0 )
    error = 1;

  {
  Display *display = 0;
  int len = 0;
  int monitors = 0;

  disp->display = XOpenDisplay (disp->name);

  /* switch to Xinerama mode */
  if( !disp->display ) {
    char *text = oyChangeScreenName_( disp->name, 0 );
    oyPostAllocHelper_m_( text, 1,
                          WARNc_S(_("allocation failed"));return 0 )

    disp->display = XOpenDisplay( text );
    oyFree_m_( text );

    if( !disp->display )
      oyPostAllocHelper_m_( disp->display, 1,
                            WARNc_S(_("open X Display failed")); return 0 )

    disp->screen = 0;
  }
  display = oyMonitor_device_( disp );

  if( !display || (len = ScreenCount( display )) <= 0 )
  {
    WARNc2_S("%s: \"%s\"", _("no Screen found"),
                           oyNoEmptyString_m_(disp->name));
    return 0;
  }

  disp->info_source = oyX11INFO_SOURCE_SCREEN;

  if(len == 1)
  {
# if HAVE_XRANDR
    int major_versionp = 0;
    int minor_versionp = 0;
    int i, n = 0;
    XRRQueryVersion( display, &major_versionp, &minor_versionp );

    if((major_versionp*100 + minor_versionp) >= 102)
    {
      /* too expensive
      Time xrr_config_time0 = 0,
           xrr_config_time = XRRTimes( display, oyMonitor_screen_(disp),
                                       &xrr_config_time0 );

      // expensive too: XRRConfigTimes()
      */
    }

    if((major_versionp*100 + minor_versionp) >= 102 && expensive)
    {
      Window w = RootWindow(display, oyMonitor_screen_(disp));
      XRRScreenResources * res = 0;
      int selected_screen = oyMonitor_getScreenFromDisplayName_( disp );

      /* a havily expensive call */
      DBG_NUM_S("going to call XRRGetScreenResources()");
      res = XRRGetScreenResources(display, w);
      DBG_NUM_S("end of call XRRGetScreenResources()");
      if(res)
        n = res->noutput;
      for(i = 0; i < n; ++i)
      {
        XRRScreenResources * res_temp = res ? res : disp->res;
        XRROutputInfo * output_info = XRRGetOutputInfo( display, res_temp,
                                                        res_temp->outputs[i]);
 
        /* we work on connected outputs */
        if( output_info && output_info->crtc )
        {
          XRRCrtcGamma * gamma = 0;

          if(monitors == 0)
          {
            gamma = XRRGetCrtcGamma( display, output_info->crtc );
            if(gamma && gamma->size &&
               strcmp("default", output_info->name) != 0)
            {
              disp->info_source = oyX11INFO_SOURCE_XRANDR;

              XRRFreeGamma( gamma );
            } else
            {
              XRRFreeOutputInfo( output_info );
              break;
            }
          }

          if(selected_screen == monitors &&
             oyMonitor_infoSource_( disp ) == oyX11INFO_SOURCE_XRANDR)
          {
            disp->output_info = output_info;
            disp->output = res_temp->outputs[i];
            output_info = 0;
            disp->res = res;
            res = 0;
            if(disp->output_info->name && oyStrlen_(disp->output_info->name))
              disp->system_port = oyStringCopy_( disp->output_info->name,
                                                 oyAllocateFunc_ );
          }

          ++ monitors;
        }

        if(output_info)
          XRRFreeOutputInfo( output_info );
      }

      if(res)
        XRRFreeScreenResources(res); res = 0;

      if(oyMonitor_infoSource_( disp ) == oyX11INFO_SOURCE_XRANDR)
      {
        if(monitors >= len && disp->output_info)
          disp->active_outputs = monitors;
        else
          disp->info_source = oyX11INFO_SOURCE_SCREEN;
      }
    }
# endif /* HAVE_XRANDR */

    if(oyMonitor_infoSource_( disp ) == oyX11INFO_SOURCE_SCREEN)
    {
# if HAVE_XIN
      /* test for Xinerama screens */
      if( XineramaIsActive( display ) )
      {
        int n_scr_info = 0;
        XineramaScreenInfo *scr_info = XineramaQueryScreens( display,
                                                             &n_scr_info );
        oyPostAllocHelper_m_(scr_info, n_scr_info, return 0 )

        if( n_scr_info >= 1 )
          len = n_scr_info;

        if(n_scr_info < monitors)
        {
          WARNc3_S( "%s: %d < %d", _("less Xinerama monitors than XRandR ones"),
                    n_scr_info, monitors );
        } else
          disp->info_source = oyX11INFO_SOURCE_XINERAMA;

        XFree( scr_info );
      }
# endif /* HAVE_XIN */
    }
  }
  }

  for( i = 0; i < 6; ++i )
    disp->geo[i] = -1;

  if( !error &&
      oyMonitor_getScreenGeometry_( disp ) != 0 )
    error = 1;

  if( error <= 0 )
    error = oyMonitor_getGeometryIdentifier_( disp );

  if( !disp->system_port || !oyStrlen_( disp->system_port ) )
  if( 0 <= oyMonitor_screen_( disp ) && oyMonitor_screen_( disp ) < 10000 )
  {
    disp->system_port = (char*)oyAllocateWrapFunc_( 12, oyAllocateFunc_ );
    oySprintf_( disp->system_port, "%d", oyMonitor_screen_( disp ) );
  }

  DBG_PROG_ENDE
  return disp;
}

#elif defined(__APPLE__)

/** @internal
 *  @brief create a monitor information struct for a given display name
 *
 *  @param   display_name              Oyranos display name
 *  @param   expensive                 probe XRandR even if it causes flickering
 *  @return                            a monitor information structure
 *
 *  @version Oyranos: 0.1.10
 *  @since   2007/12/17 (Oyranos: 0.1.8)
 *  @date    2009/04/01
 */
oyMonitor_s* oyMonitor_newFrom_      ( const char        * display_name,
                                       int                 expensive )
{
  int error = 0;
  int i = 0;
  oyMonitor_s * disp = 0;

  DBG_PROG_START

  disp = oyAllocateFunc_( sizeof(oyMonitor_s) );
  error = !disp;
  if(!error)
    error = !memset( disp, 0, sizeof(oyMonitor_s) );

  disp->type_ = oyOBJECT_MONITOR_S;

  if( display_name )
  {
    if( display_name[0] )
      disp->name = oyStringCopy_( display_name, oyAllocateFunc_ );
  } else
  {
    if(getenv("DISPLAY") && strlen(getenv("DISPLAY")))
      disp->name = oyStringCopy_( getenv("DISPLAY"), oyAllocateFunc_ );
    else
      disp->name = oyStringCopy_( "0", oyAllocateFunc_ );
  }

  if( !error &&
      (disp->host = oyStringCopy_( "0", oyAllocateFunc_ )) == 0 )
    error = 1;

  for( i = 0; i < 6; ++i )
    disp->geo[i] = -1;

  disp->id = oyMonitor_nameToOsxID( display_name );

  if( !error &&
      oyMonitor_getScreenGeometry_( disp ) != 0 )
    error = 1;

  if( error <= 0 )
    error = oyMonitor_getGeometryIdentifier_( disp );

  /*if( !disp->system_port || !oyStrlen_( disp->system_port ) )
  if( 0 <= oyMonitor_screen_( disp ) && oyMonitor_screen_( disp ) < 10000 )
  {
    disp->system_port = (char*)oyAllocateWrapFunc_( 12, oyAllocateFunc_ );
    oySprintf_( disp->system_port, "%d", oyMonitor_screen_( disp ) );
  }*/

  DBG_PROG_ENDE
  return disp;
}

/** @internal get the geometry of a screen 
 *
 *  @param          disp      display info structure
 *  @return         error
 *
 *  @version Oyranos: 0.1.10
 *  @since   2007/00/00 (Oyranos: 0.1.8)
 *  @date    2009/04/01
 */
int  oyMonitor_getScreenGeometry_    ( oyMonitor_s       * disp )
{
  int error = 0;

  {
    CGRect            r = CGRectNull;

    disp->geo[0] = 0; /* @todo how is display management handled in osX? */
    disp->geo[1] = disp->screen;

    r = CGDisplayBounds( oyMonitor_device_( disp ) );

    disp->geo[2] = r.origin.x;
    disp->geo[3] = r.origin.y;
    disp->geo[4] = r.size.width;
    disp->geo[5] = r.size.height;
  }

  return error;
}
#endif

/** @internal
 *  @brief release a monitor information stuct
 *
 *  @version Oyranos: 0.1.10
 *  @since   2007/12/17 (Oyranos: 0.1.8)
 *  @date    2009/04/01
 */
int          oyMonitor_release_      ( oyMonitor_s      ** obj )
{
  int error = 0;
  /* ---- start of common object destructor ----- */
  oyMonitor_s * s = 0;
  
  if(!obj || !*obj)
    return 0;
  
  s = *obj;
  
  if( s->type_ != oyOBJECT_MONITOR_S)
  { 
    WARNc_S("Attempt to release a non oyMonitor_s object.")
    return 1;
  }
  /* ---- end of common object destructor ------- */

  if(s->name) oyDeAllocateFunc_( s->name );
  if(s->host) oyDeAllocateFunc_( s->host );
  if(s->identifier) oyDeAllocateFunc_( s->identifier );


  s->geo[0] = s->geo[1] = -1;

# if !defined(__APPLE__)
  if( s->display )
  {
#  ifdef HAVE_XRANDR
    if(s->output_info)
      XRRFreeOutputInfo( s->output_info ); s->output_info = 0;
    if(s->res)
      XRRFreeScreenResources( s->res ); s->res = 0;
#  endif
    XCloseDisplay( s->display );
    s->display=0;
  }
# endif

  oyDeAllocateFunc_( s );
  s = 0;

  *obj = 0;

  return error; 
}


/* separate from the internal functions */

/** @brief pick up monitor information with Xlib
 *  @deprecated because sometimes is no ddc information available
 *  @todo include connection information - grafic cart
 *
 *  @param      display       the display string
 *  @param[out] manufacturer  the manufacturer of the monitor device
 *  @param[out] model         the model of the monitor device
 *  @param[out] serial        the serial number of the monitor device
 *  @param      allocate_func the allocator for the above strings
 *  @return     error
 *
 */
int
oyGetMonitorInfo_lib              (const char* display,
                                   char**      manufacturer,
                                   char**      model,
                                   char**      serial,
                                       char             ** display_geometry,
                                       char             ** system_port,
                                       char             ** host,
                                       oyBlob_s         ** edid,
                                   oyAlloc_f     allocate_func,
                                       oyStruct_s        * user_data)
{
  int err = 0;

  DBG_PROG_START

#if (defined(HAVE_X) && !defined(__APPLE__))
  err = oyGetMonitorInfo_( display, manufacturer, model, serial,
                     display_geometry, system_port, host, edid,
                     allocate_func, user_data );
#else
  err = 1;
#endif

  if(*manufacturer)
    DBG_PROG_S( *manufacturer );
  if(*model)
    DBG_PROG_S( *model );
  if(*serial)
    DBG_PROG_S( *serial );

  DBG_PROG_ENDE
  return err;
}




