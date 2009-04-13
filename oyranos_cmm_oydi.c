/** @file oyranos_cmm_oydi.c
 *
 *  Oyranos is an open source Colour Management System 
 *
 *  @par Copyright:
 *            2009 (C) Kai-Uwe Behrmann
 *
 *  @brief    Oyranos X11 backend for Oyranos
 *  @internal
 *  @author   Kai-Uwe Behrmann <ku.b@gmx.de>
 *  @par License:
 *            new BSD <http://www.opensource.org/licenses/bsd-license.php>
 *  @since    2009/02/23
 */

#include "oyranos_cmm.h"
#include "oyranos_debug.h"
#include "oyranos_helper.h"
#include "oyranos_i18n.h"
#include "oyranos_monitor.h"
#include "oyranos_texts.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>


/* --- internal definitions --- */

#define CMM_NICK "oydi"
#define CMM_VERSION {OYRANOS_VERSION_A,OYRANOS_VERSION_B,OYRANOS_VERSION_C}

/* OY_IMAGE_DISPLAY_REGISTRATION */
/* OYX1_MONITOR_REGISTRATION */

int                oydiCMMInit       ( );
int            oydiCMMMessageFuncSet ( oyMessage_f         message_func );
int            oydiCMMCanHandle      ( oyCMMQUERY_e        type,
                                       uint32_t            value );
oyMessage_f message = 0;


oyWIDGET_EVENT_e   oydiWidgetEvent   ( oyOptions_s       * options,
                                       oyWIDGET_EVENT_e    type,
                                       oyStruct_s        * event );
int  oydiFilterSocket_ImageDisplayInit(oyFilterSocket_s  * socket,
                                       oyImage_s         * image );
oyOptions_s* oydiFilter_ImageDisplayValidateOptions
                                     ( oyFilterCore_s    * filter,
                                       oyOptions_s       * validate,
                                       int                 statical,
                                       uint32_t          * result );

extern oyCMMapi4_s   oydi_api4_image_display;
extern oyCMMapi7_s   oydi_api7_image_display;



/* OY_IMAGE_DISPLAY_REGISTRATION ---------------------------------------------*/

#ifdef HAVE_X11
#include <X11/extensions/Xfixes.h>
#endif
#ifdef HAVE_Xcolor
#include <Xcolor.h>
#endif

/** @func    oydiFilter_ImageDisplayCanHandle
 *  @brief   inform about image handling capabilities
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/02/23 (Oyranos: 0.1.10)
 *  @date    2009/03/04
 */
int    oydiFilter_ImageDisplayCanHandle(oyCMMQUERY_e     type,
                                       uint32_t          value )
{
  int ret = -1;

  switch(type)
  {
    case oyQUERY_OYRANOS_COMPATIBILITY:
         ret = OYRANOS_VERSION; break;
    case oyQUERY_PIXELLAYOUT_CHANNELCOUNT:
         ret = 65535;
         break;
    case oyQUERY_PIXELLAYOUT_DATATYPE:
         switch(value) {
         case oyUINT8:
         case oyUINT16:
         case oyUINT32:
         case oyHALF:
         case oyFLOAT:
         case oyDOUBLE:
              ret = 1; break;
         default:
              ret = 0; break;
         }
         break;
    case oyQUERY_PIXELLAYOUT_SWAP_COLOURCHANNELS:
         ret = 1;
         break;
    case oyQUERY_PIXELLAYOUT_PLANAR:
         ret = 1;
         break;
    case oyQUERY_HDR:
         ret = 1;
         break;
    default: break;
  }

  return ret;
}

oyWIDGET_EVENT_e   oydiWidgetEvent   ( oyOptions_s       * options,
                                       oyWIDGET_EVENT_e    type,
                                       oyStruct_s        * event )
{return 0;}

oyOptions_s* oydiFilter_ImageDisplayValidateOptions
                                     ( oyFilterCore_s    * filter,
                                       oyOptions_s       * validate,
                                       int                 statical,
                                       uint32_t          * result )
{
  uint32_t error = !filter;

  if(!error)
    error = filter->type_ != oyOBJECT_FILTER_CORE_S;

  *result = error;

  return 0;
}

/** @func    oydiFilterNode_ImageDisplayContextToMem
 *  @brief   implement oyCMMFilter_ContextToMem_f()
 *
 *  Serialise into a Oyranos specific ICC profile containers "Info" tag.
 *  We do not have any binary context to include.
 *  Thus oyFilterNode_TextToInfo_() is fine.
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/02/23 (Oyranos: 0.1.10)
 *  @date    2009/03/04
 */
oyPointer  oydiFilterNode_ImageDisplayContextToMem (
                                       oyFilterNode_s    * node,
                                       size_t            * size,
                                       oyAlloc_f           allocateFunc )
{
  return oyFilterNode_TextToInfo_( node, size, allocateFunc );
}

char *   oydiFilterNode_ImageDisplayID(oyFilterNode_s    * node )
{
  char * ID = malloc(128);

  sprintf( ID, "//image/display/filter_id_%d", oyObject_GetId( node->oy_ ));

  return ID;
}

int oydiFilterSocket_SetWindowRegion ( oyFilterSocket_s  * socket,
                                       oyImage_s         * image )
{
  int error = 0;
  oyBlob_s * win_id = 0;

  win_id = (oyBlob_s*) oyOptions_GetType( image->tags, -1, "window_id",
                                            oyOBJECT_BLOB_S );

# if defined(HAVE_X11) && defined (HAVE_Xcolor)
  if(win_id)
  {
    Atom netColorTarget;
    Window w = (Window) win_id->ptr;
    const char * display_name = oyOptions_FindString( image->tags,
                                                      "display_name", 0 );
    Display * display = XOpenDisplay( display_name );
    oyRegion_s * old_display_region = (oyRegion_s*) oyOptions_GetType( 
                                      image->tags, -1,
                                      "old_display_region", oyOBJECT_REGION_S );
    oyRegion_s * display_region = (oyRegion_s*) oyOptions_GetType( 
                                      image->tags, -1,
                                      "display_region", oyOBJECT_REGION_S );

      WARNc4_S("  Display: %s Window id: %d  %s %s",
               display_name, w, oyRegion_Show(display_region),
               oyRegion_Show( old_display_region ) );

    oyBlob_Release( &win_id );

    if(!old_display_region)
    {
      old_display_region = oyRegion_NewFrom( 0,0 );

      oyOptions_MoveInStruct( &image->tags,
                            "//image/display/old_display_region",
                            (oyStruct_s**) &old_display_region, OY_CREATE_NEW );
      old_display_region = (oyRegion_s*) oyOptions_GetType(
                                      image->tags, -1,
                                      "old_display_region", oyOBJECT_REGION_S );
    }

    if(!oyRegion_IsEqual( display_region, old_display_region ))
    {
      /* Upload the region to the window. */
      XRectangle rec[2] = { { 0,0,0,0 }, { 0,0,0,0 } };
      XserverRegion reg = 0;
      XcolorRegion region;

      WARNc3_S("  Display: %s Window id: %d  %s",
               display_name, w, oyRegion_Show(display_region) );
      rec[0].x = display_region->x;
      rec[0].y = display_region->y;
      rec[0].width = display_region->width;
      rec[0].height = display_region->height;

      reg = XFixesCreateRegion( display, rec, 1);

      region.region = reg;
      memset( region.md5, 0, 16 );

      error = XcolorRegionInsert( display, w, 0, &region, 1 );
      netColorTarget = XInternAtom( display, "_NET_COLOR_TARGET", True );
      XChangeProperty( display, w, netColorTarget, XA_STRING, 8,
                       PropModeReplace,
                       (unsigned char *) ":0.0", strlen(":0.0") );

    }
    XClearWindow( display, w );
    oyRegion_Release( &display_region );
    oyRegion_Release( &old_display_region );
  }
# endif

  return error;
}

int  oydiFilterSocket_ImageDisplayInit(oyFilterSocket_s  * socket,
                                       oyImage_s         * image )
{
  int n = 0, i,m;
  int error = 0;
  oyFilterGraph_s * display_graph = 0;
  oyFilterNode_s * input_node = 0,
                 * node = socket->node,
                 * cmm_node = 0,
                 * regions = 0;
  oyOptions_s * options = 0;
  oyOption_s * o = 0;
  oyRegion_s * r;
  oyConfigs_s * devices = 0;
  char * tmp = 0,
       * ID = 0;

  if(oy_debug) WARNc_S("Init Start");

  input_node = node->plugs[0]->remote_socket_->node;

  ID = oydiFilterNode_ImageDisplayID( node );

  /* insert a "regions" filter to handle multiple monitors */
  regions = oyFilterNode_NewWith( "//image/regions", 0,0, 0 );
  /* mark the new node as belonging to this node */
  oyOptions_SetFromText( &regions->tags, ID, "true", OY_CREATE_NEW );

  /* insert "regions" between "display" and its input_node */
  oyFilterNode_Disconnect( node->plugs[0] );
  error = oyFilterNode_Connect( input_node, "Img", regions, "Img",0 );
  error = oyFilterNode_Connect( regions, "Img", node, "Img",0 );



  /* obtain device informations, including geometry and ICC profiles
     from the according Oyranos backend */
  error = oyOptions_SetFromText( &options, "//colour/config/list",
                                 "true", OY_CREATE_NEW );
  error = oyOptions_SetFromText( &options,
                                 "//colour/config/device_region",
                                 "true", OY_CREATE_NEW );
  o = oyOptions_Find( image->tags, "display_name" );
  o = oyOption_Copy( o, 0 );
  oyOptions_MoveIn( options, &o, -1 );
  error = oyDevicesGet( "colour", "monitor", options, &devices );
  n = oyConfigs_Count( devices );
  o = oyOptions_Find( node->core->options_, "devices" );
  /* cache the devices scan result; currently is no updating implemented */
  if(!o)
  {
    oyOptions_MoveInStruct( &node->core->options_,
                            "//image/display/devices",
                            (oyStruct_s**) &devices, OY_CREATE_NEW );
  }
  else
  {
    oyConfigs_Release( (oyConfigs_s**) & o->value->oy_struct );
    o->value->oy_struct = (oyStruct_s*) oyConfigs_Copy( devices, 0 );
  }
  oyOption_Release( &o );


  m = oyFilterNode_EdgeCount( regions, 1, OY_FILTEREDGE_CONNECTED );

  /* add new regions and colour CMMs as needed */
  if(n > m)
  {
    for(i = 0; i < n-m; ++i)
    {
      /* The first region is the one provided by the user graph. */
      if(m != 0 || i != 0)
      {
        if(oyFilterRegistrationMatch( input_node->core->registration_,
                                      "//colour/icc", 0 ))
          cmm_node = oyFilterNode_NewWith( input_node->core->registration_,
                                           0,0, 0 );
        else
          WARNc2_S("\n  Filter %s expects a colour conversion filter as"
                   " input\n  But obtained: %s", node->relatives_,
                   input_node->relatives_ );

        /* mark the new node as belonging to this node */
        oyOptions_SetFromText( &cmm_node->tags, ID, "true",
                               OY_CREATE_NEW );

        /* position the new CMM between the original CMMs input and 
           "regions" */
        error = oyFilterNode_Connect( cmm_node, "Img",
                                      regions, "Img", 0 );
        if(error > 0)
          WARNc1_S( "could not add  new CMM: %s\n",
                    input_node->core->registration_ );

        error = oyFilterNode_Connect( 
                                  input_node->plugs[0]->remote_socket_->node, 0,
                                      cmm_node, "Img", 0 );

        /* clone into a new image */
        if(cmm_node->sockets[0]->data)
          cmm_node->sockets[0]->data->release( 
                                    (oyStruct_s**)&cmm_node->sockets[0]->data );
        cmm_node->sockets[0]->data = (oyStruct_s*)oyImage_CreateForDisplay (
                                                image->width, image->height,
                                                0, image->layout_[0],
                                                0, 0,0,0,0, 0 );
      }
    }
  }


  m = oyOptions_CountType( regions->core->options_,
                           "//image/regions/region", oyOBJECT_REGION_S );
  /* add missed regions */
  if(n > m)
  {
    tmp = oyAllocateFunc_(64);

    for(i = m; i < n;  ++i)
    {
      oySprintf_( tmp, "//image/regions/region/%d", i );

      r = oyRegion_NewWith( 0., 0., 0., 0., 0);
      oyOptions_MoveInStruct( &regions->core->options_, tmp,
                              (oyStruct_s**)&r, OY_CREATE_NEW );
    }

    oyDeAllocateFunc_(tmp); tmp = 0;
  }


  /* describe all our newly created filters and add them to this node */
  display_graph = oyFilterGraph_New( 0 );
  oyFilterGraph_SetFromNode( display_graph, regions, ID, 0 );
  oyOptions_MoveInStruct( &node->core->options_,
                          "//image/display/display_graph",
                          (oyStruct_s**) &display_graph, OY_CREATE_NEW );



  if(oy_debug) WARNc_S("Init End");

  free(ID); ID = 0;

  return error;
}

/** @func    oydiFilterPlug_ImageDisplayRun
 *  @brief   implement oyCMMFilter_GetNext_f()
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/02/23 (Oyranos: 0.1.10)
 *  @date    2009/03/17
 */
int      oydiFilterPlug_ImageDisplayRun(oyFilterPlug_s   * requestor_plug,
                                       oyPixelAccess_s   * ticket )
{
  int x = 0, y = 0, n = 0, i;
  int result = 0, error = 0;
  oyFilterGraph_s * display_graph = 0;
  oyFilterSocket_s * socket = requestor_plug->remote_socket_;
  oyFilterNode_s * node = socket->node,
                 * regions = 0;
  oyImage_s * image = (oyImage_s*)socket->data,
            * input_image = 0;
  oyOption_s * o = 0;
  oyRegion_s * r, * rd, * ri;
  oyConfigs_s * devices = 0;
  oyConfig_s * c = 0;
  oyProfile_s * p = 0;
  int display_pos_x,
      display_pos_y;
  int dirty = 0,
      init = 0;
  char * ID = 0;

  x = ticket->start_xy[0];
  y = ticket->start_xy[1];

  result = oyPixelAccess_CalculateNextStartPixel( ticket, requestor_plug);

  if(result != 0)
    return result;

  ID = oydiFilterNode_ImageDisplayID( node );

  {
    /* display stuff */

    if(!node->core->options_)
      /* allocate options */
      node->core->options_ = oyOptions_New(0);

    /* obtain the local graph */
    display_graph = (oyFilterGraph_s*)oyOptions_GetType( node->core->options_,
                                            -1, "//image/display/display_graph",
                                            oyOBJECT_FILTER_GRAPH_S );

    if(!display_graph)
    {
      init = 1;

      /* init this filter */
      oydiFilterSocket_ImageDisplayInit( socket, image );

      display_graph = (oyFilterGraph_s*)oyOptions_GetType( node->core->options_,
                                            -1, "//image/display/display_graph",
                                            oyOBJECT_FILTER_GRAPH_S );
      error = !display_graph;
    }

    /* set server side region */
    oydiFilterSocket_SetWindowRegion( socket, image );

    /* look for our requisites */
    regions = oyFilterGraph_GetNode( display_graph, -1, "//image/regions", ID );

    /* get cached devices */
    devices = (oyConfigs_s*)oyOptions_GetType( node->core->options_, -1, 
                                "//image/display/devices", oyOBJECT_CONFIGS_S );

    n = oyConfigs_Count( devices );
    if(!n || oyFilterNode_EdgeCount( regions, 1, OY_FILTEREDGE_CONNECTED ) < n)
      return 1;

    /* process all display regions */
    if(error <= 0)
    for(i = 0; i < n; ++i)
    {
      c = oyConfigs_Get( devices, i );

      /* get device dimension */
      o = oyConfig_Find( c, "device_region" );
      if(o && o->value_type == oyVAL_STRUCT)
        rd = (oyRegion_s *) o->value->oy_struct;
      oyOption_Release( &o );

      /* get current work region */
      r = (oyRegion_s *) oyOptions_GetType( regions->core->options_, i, 
                                  "//image/regions/region", oyOBJECT_REGION_S );

      /* get display region to project into */
      o = oyOptions_Find( image->tags, "display_region" );
      if(o && o->value_type == oyVAL_STRUCT && o->value &&
         o->value->oy_struct->type_ == oyOBJECT_REGION_S)
        ri = (oyRegion_s *) o->value->oy_struct;
      oyOption_Release( &o );

      /* trim and adapt the work region */
      oyRegion_SetByRegion( r, ri );
      display_pos_x = r->x;
      display_pos_y = r->y;
      oyRegion_Trim( r, rd );
      r->x -= display_pos_x;
      r->y -= display_pos_y;
      if(oy_debug) WARNc2_S("image %d: %s", i, oyRegion_Show( r ));

      /* all regions are relative to image dimensions */
      if(image->width != 0)
        oyRegion_Scale( r, 1./image->width );

      /* select actual image from the according CMM node */
      if(regions->plugs && regions->plugs[i] &&
         regions->plugs[i]->remote_socket_)
        input_image = (oyImage_s*)regions->plugs[i]->remote_socket_->data;
      else
      {
        input_image = 0;
        WARNc2_S("image %d: is missed", i, oyRegion_Show( r ));
      }

      /* set the device profile of all CMM's image data */
      if(init)
      {
        error = oyDeviceGetProfile( c, &p );

        if(p && input_image && !oyProfile_Equal( input_image->profile_, p ))
        {
          oyImage_SetCritical( input_image, 0, p, 0 );
          error = oyOptions_SetFromText( &ticket->graph->options,
                               "//image/profile/dirty", "true", OY_CREATE_NEW );
          ++dirty;
        }

        oyProfile_Release( &p );
      }

      oyConfig_Release( &c );
    }

    oyConfigs_Release( &devices );

    /* stop here and request an update */
    if(dirty > 0)
    {
      oyFilterNode_Release( &regions );
      result = dirty;
      goto clean;
    }


    /* make the graph flow: process the upstream "regions" node */
    regions->api7_->oyCMMFilterPlug_Run( node->plugs[0], ticket );

  }

  clean:
  oyFilterNode_Release( &regions );
  if(ID) free(ID);    

  return result;
}

oyDATATYPE_e oyx1_data_types[7] = {oyUINT8, oyUINT16, oyUINT32,
                                         oyHALF, oyFLOAT, oyDOUBLE, 0};

oyConnector_s oyx1_Display_plug = {
  oyOBJECT_CONNECTOR_S,0,0,0,
  {oyOBJECT_NAME_S, 0,0,0, "Img", "Image", "Image Display Plug"},
  oyCONNECTOR_SPLITTER, /* connector_type */
  1, /* is_plug == oyFilterPlug_s */
  oyx1_data_types, /* data_types */
  6, /* data_types_n; elements in data_types array */
  -1, /* max_colour_offset */
  1, /* min_channels_count; */
  255, /* max_channels_count; */
  1, /* min_colour_count; */
  255, /* max_colour_count; */
  0, /* can_planar; can read separated channels */
  1, /* can_interwoven; can read continuous channels */
  1, /* can_swap; can swap colour channels (BGR)*/
  1, /* can_swap_bytes; non host byte order */
  1, /* can_revert; revert 1 -> 0 and 0 -> 1 */
  1, /* can_premultiplied_alpha; */
  1, /* can_nonpremultiplied_alpha; */
  1, /* can_subpixel; understand subpixel order */
  0, /* oyCHANNELTYPE_e    * channel_types; */
  0, /* count in channel_types */
  1, /* id; relative to oyFilter_s, e.g. 1 */
  0  /* is_mandatory; mandatory flag */
};
oyConnector_s *oyx1_Display_plugs[2] = {&oyx1_Display_plug,0};

oyConnector_s oyx1_Display_socket = {
  oyOBJECT_CONNECTOR_S,0,0,0,
  {oyOBJECT_NAME_S, 0,0,0, "Img", "Image", "Image Display Plug"},
  oyCONNECTOR_IMAGE, /* connector_type */
  0, /* is_plug == oyFilterPlug_s */
  oyx1_data_types, /* data_types */
  6, /* data_types_n; elements in data_types array */
  -1, /* max_colour_offset */
  1, /* min_channels_count; */
  255, /* max_channels_count; */
  1, /* min_colour_count; */
  255, /* max_colour_count; */
  0, /* can_planar; can read separated channels */
  1, /* can_interwoven; can read continuous channels */
  1, /* can_swap; can swap colour channels (BGR)*/
  1, /* can_swap_bytes; non host byte order */
  1, /* can_revert; revert 1 -> 0 and 0 -> 1 */
  1, /* can_premultiplied_alpha; */
  1, /* can_nonpremultiplied_alpha; */
  1, /* can_subpixel; understand subpixel order */
  0, /* oyCHANNELTYPE_e    * channel_types; */
  0, /* count in channel_types */
  2, /* id; relative to oyFilter_s, e.g. 1 */
  0  /* is_mandatory; mandatory flag */
};
oyConnector_s *oyx1_Display_sockets[2] = {&oyx1_Display_socket,0};


#define OY_IMAGE_DISPLAY_REGISTRATION OY_TOP_INTERNAL OY_SLASH OY_DOMAIN_INTERNAL OY_SLASH "image/display"
/** @instance oydi_api7_image_display
 *  @brief    oydi oyCMMapi7_s implementation
 *
 *  a filter for expanding the graph to several display devices
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/02/24 (Oyranos: 0.1.10)
 *  @date    2009/03/04
 */
oyCMMapi7_s   oydi_api7_image_display = {

  oyOBJECT_CMM_API7_S, /* oyStruct_s::type oyOBJECT_CMM_API7_S */
  0,0,0, /* unused oyStruct_s fileds; keep to zero */
  (oyCMMapi_s*) 0, /* oyCMMapi_s * next */
  
  oydiCMMInit, /* oyCMMInit_f */
  oydiCMMMessageFuncSet, /* oyCMMMessageFuncSet_f */
  oydiCMMCanHandle, /* oyCMMCanHandle_f */

  /* registration */
  OY_IMAGE_DISPLAY_REGISTRATION,

  CMM_VERSION, /* int32_t version[3] */
  0,   /* id_; keep empty */
  0,   /* api5_; keep empty */

  oydiFilterPlug_ImageDisplayRun, /* oyCMMFilterPlug_Run_f */
  {0}, /* char data_type[8] */

  oyx1_Display_plugs,   /* plugs */
  1,   /* plugs_n */
  0,   /* plugs_last_add */
  oyx1_Display_sockets,   /* sockets */
  1,   /* sockets_n */
  0    /* sockets_last_add */
};

/** @instance oydi_api4_image_display
 *  @brief    oydi oyCMMapi4_s implementation
 *
 *  a filter for expanding the graph to several display devices
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/02/24 (Oyranos: 0.1.10)
 *  @date    2009/03/04
 */
oyCMMapi4_s   oydi_api4_image_display = {

  oyOBJECT_CMM_API4_S, /* oyStruct_s::type oyOBJECT_CMM_API4_S */
  0,0,0, /* unused oyStruct_s fileds; keep to zero */
  (oyCMMapi_s*) & oydi_api7_image_display, /* oyCMMapi_s * next */
  
  oydiCMMInit, /* oyCMMInit_f */
  oydiCMMMessageFuncSet, /* oyCMMMessageFuncSet_f */
  oydiCMMCanHandle, /* oyCMMCanHandle_f */

  /* registration */
  OY_IMAGE_DISPLAY_REGISTRATION,

  CMM_VERSION, /* int32_t version[3] */
  0,   /* id_; keep empty */
  0,   /* api5_; keep empty */

  oydiFilter_ImageDisplayValidateOptions, /* oyCMMFilter_ValidateOptions_f */
  oydiWidgetEvent, /* oyWidgetEvent_f */

  oydiFilterNode_ImageDisplayContextToMem, /* oyCMMFilterNode_ContextToMem_f */
  0, /* oyCMMFilterNode_ContextToMem_f oyCMMFilterNode_ContextToMem */
  {0}, /* char context_type[8] */

  {oyOBJECT_NAME_S, 0,0,0, "display", "Display", "Display Splitter Object"}, /* name; translatable, eg "scale" "image scaling" "..." */
  "Graph/Display", /* category */
  0,   /* options */
  0    /* opts_ui_ */
};

/* OY_IMAGE_DISPLAY_REGISTRATION ---------------------------------------------*/



/** Function oydiCMMInit
 *  @brief   API requirement
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/04/13 (Oyranos: 0.1.10)
 *  @date    2009/04/13
 */
int                oydiCMMInit       ( )
{
  int error = 0;
  return error;
}

/** Function oydiCMMMessageFuncSet
 *  @brief   API requirement
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/04/13 (Oyranos: 0.1.10)
 *  @date    2009/04/13
 */
int            oydiCMMMessageFuncSet ( oyMessage_f         message_func )
{
  message = message_func;
  return 0;
}

/** Function oydiCMMCanHandle
 *  @brief   API requirement
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/04/13 (Oyranos: 0.1.10)
 *  @date    2009/04/13
 */
int            oydiCMMCanHandle      ( oyCMMQUERY_e        type,
                                       uint32_t            value ) {return 0;}

/**
 *  This function implements oyCMMInfoGetText_f.
 *
 *  @version Oyranos: 0.1.10
 *  @since   2008/12/23 (Oyranos: 0.1.10)
 *  @date    2008/04/12
 */
const char * oydiGetText             ( const char        * select,
                                       oyNAME_e            type )
{
         if(strcmp(select, "name")==0)
  {
         if(type == oyNAME_NICK)
      return _(CMM_NICK);
    else if(type == oyNAME_NAME)
      return _("Oyranos X11");
    else
      return _("The client side window data handler of Oyranos.");
  } else if(strcmp(select, "manufacturer")==0)
  {
         if(type == oyNAME_NICK)
      return _("Kai-Uwe");
    else if(type == oyNAME_NAME)
      return _("Kai-Uwe Behrmann");
    else
      return _("Oyranos project; www: http://www.oyranos.com; support/email: ku.b@gmx.de; sources: http://www.oyranos.com/wiki/index.php?title=Oyranos/Download");
  } else if(strcmp(select, "copyright")==0)
  {
         if(type == oyNAME_NICK)
      return _("newBSD");
    else if(type == oyNAME_NAME)
      return _("Copyright (c) 2009 Kai-Uwe Behrmann; newBSD");
    else
      return _("new BSD license: http://www.opensource.org/licenses/bsd-license.php");
  }
  return 0;
}

/** @instance oydi_cmm_module
 *  @brief    oydi module infos
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/01/00 (Oyranos: 0.1.10)
 *  @date    2009/01/00
 */
oyCMMInfo_s oydi_cmm_module = {

  oyOBJECT_CMM_INFO_S,
  0,0,0,
  CMM_NICK,
  "0.2",
  oydiGetText, /* oyCMMInfoGetText_f */
  OYRANOS_VERSION,

  (oyCMMapi_s*) & oydi_api4_image_display,

  {oyOBJECT_ICON_S, 0,0,0, 0,0,0, "oyranos_logo.png"},
};
