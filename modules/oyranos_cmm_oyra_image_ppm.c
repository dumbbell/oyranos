/** @file oyranos_cmm_oyra_image.c
 *
 *  Oyranos is an open source Colour Management System 
 *
 *  @par Copyright:
 *            2008-2009 (C) Kai-Uwe Behrmann
 *
 *  @brief    modules for Oyranos
 *  @internal
 *  @author   Kai-Uwe Behrmann <ku.b@gmx.de>
 *  @par License:
 *            new BSD <http://www.opensource.org/licenses/bsd-license.php>
 *  @since    2008/10/07
 */

#include "config.h"
#include "oyranos_alpha.h"
#include "oyranos_alpha_internal.h"
#include "oyranos_cmm.h"
#include "oyranos_cmm_oyra.h"
#include "oyranos_helper.h"
#include "oyranos_icc.h"
#include "oyranos_i18n.h"
#include "oyranos_io.h"
#include "oyranos_definitions.h"
#include "oyranos_string.h"
#include "oyranos_texts.h"
#include <iconv.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int wread ( unsigned char   *data,    /* read a word */
            size_t  pos,
            size_t  max,
            size_t *start,
            size_t *length );

oyOptions_s* oyraFilter_ImageOutputPPMValidateOptions
                                     ( oyFilterCore_s    * filter,
                                       oyOptions_s       * validate,
                                       int                 statical,
                                       uint32_t          * result )
{
  uint32_t error = !filter;

#if 0
  oyDATATYPE_e data_type = 0;
  int planar, channels;
  oyImage_s * image = 0;

  if(!error)
    filter = node->filter;

  if(!error)
    error = filter->type_ != oyOBJECT_FILTER_S;

  if(!error)
  {
    if(filter->image_ && filter->image_->layout_)
    {
      data_type = oyToDataType_m( filter->image_->layout_[0] );
      if(!(data_type == oyUINT8 ||
           data_type == oyUINT16 ||
           data_type == oyFLOAT ||
           data_type == oyDOUBLE
                                   ))
        error = 1;

      planar = oyToPlanar_m( filter->image_->layout_[0] );
      if(!error && planar)
        error = 1;

      channels = oyToChannels_m( filter->image_->layout_[0] );
      if(!error && channels > 4)
        error = 1;
    }
  }
#endif

  if(!error)
    error = !oyOptions_FindString( validate, "filename", 0 );

  *result = error;

  return 0;
}

/** @func    oyraFilterPlug_ImageOutputPPMWrite
 *  @brief   implement oyCMMFilter_GetNext_f()
 *
 *  @version Oyranos: 0.3.1
 *  @since   2008/10/07 (Oyranos: 0.1.8)
 *  @date    2011/05/12
 */
int      oyraFilterPlug_ImageOutputPPMWrite (
                                       oyFilterPlug_s    * requestor_plug,
                                       oyPixelAccess_s   * ticket )
{
  oyFilterSocket_s * socket = requestor_plug->remote_socket_;
  oyFilterPlug_s * plug = 0;
  oyFilterNode_s * input_node = 0,
                 * node = 0;
  int result = 0;
  const char * filename = 0;

  node = socket->node;
  plug = (oyFilterPlug_s *)node->plugs[0];
  input_node = plug->remote_socket_->node;

  /* to reuse the requestor_plug is a exception for the starting request */
  result = input_node->api7_->oyCMMFilterPlug_Run( plug, ticket );

  if(result <= 0)
    filename = oyOptions_FindString( node->core->options_, "filename", 0 );

  if(filename && socket)
  {
    oyImage_s *image_output = (oyImage_s*)socket->data;
    result = oyImage_WritePPM( image_output, filename, node->relatives_ );
  }

  return result;
}

const char ppm_write_extra_options[] = {
 "\n\
  <" OY_TOP_SHARED ">\n\
   <" OY_DOMAIN_INTERNAL ">\n\
    <" OY_TYPE_STD ">\n\
     <" "file_write" ">\n\
      <filename></filename>\n\
     </" "file_write" ">\n\
    </" OY_TYPE_STD ">\n\
   </" OY_DOMAIN_INTERNAL ">\n\
  </" OY_TOP_SHARED ">\n"
};

int  oyraPPMwriteUiGet               ( oyOptions_s       * opts,
                                       char             ** xforms_layout,
                                       oyAlloc_f           allocateFunc )
{
  char * text = (char*)allocateFunc(5);
  text[0] = 0;
  *xforms_layout = text;
  return 0;
}


oyDATATYPE_e oyra_image_ppm_data_types[5] = {oyUINT8, oyUINT16,
                                             oyFLOAT, oyDOUBLE, 0};

oyConnectorImaging_s oyra_imageOutputPPM_connector_out = {
  oyOBJECT_CONNECTOR_IMAGING_S,0,0,
                               (oyObject_s)&oy_connector_imaging_static_object,
  oyCMMgetImageConnectorSocketText, /* getText */
  oy_image_connector_texts, /* texts */
  "//" OY_TYPE_STD "/image.data", /* connector_type */
  oyFilterSocket_MatchImagingPlug, /* filterSocket_MatchPlug */
  0, /* is_plug == oyFilterPlug_s */
  oyra_image_ppm_data_types,
  4, /* data_types_n; elements in data_types array */
  -1, /* max_colour_offset */
  1, /* min_channels_count; */
  4, /* max_channels_count; */
  1, /* min_colour_count; */
  4, /* max_colour_count; */
  0, /* can_planar; can read separated channels */
  1, /* can_interwoven; can read continuous channels */
  0, /* can_swap; can swap colour channels (BGR)*/
  0, /* can_swap_bytes; non host byte order */
  0, /* can_revert; revert 1 -> 0 and 0 -> 1 */
  1, /* can_premultiplied_alpha; */
  1, /* can_nonpremultiplied_alpha; */
  0, /* can_subpixel; understand subpixel order */
  0, /* oyCHANNELTYPE_e    * channel_types; */
  0, /* count in channel_types */
  1, /* id; relative to oyFilter_s, e.g. 1 */
  0  /* is_mandatory; mandatory flag */
};
oyConnectorImaging_s * oyra_imageOutputPPM_connectors_socket[2] = 
             { &oyra_imageOutputPPM_connector_out, 0 };

oyConnectorImaging_s oyra_imageOutputPPM_connector_in = {
  oyOBJECT_CONNECTOR_IMAGING_S,0,0,
                               (oyObject_s)&oy_connector_imaging_static_object,
  oyCMMgetImageConnectorPlugText, /* getText */
  oy_image_connector_texts, /* texts */
  "//" OY_TYPE_STD "/image.data", /* connector_type */
  oyFilterSocket_MatchImagingPlug, /* filterSocket_MatchPlug */
  1, /* is_plug == oyFilterPlug_s */
  oyra_image_ppm_data_types,
  4, /* data_types_n; elements in data_types array */
  -1, /* max_colour_offset */
  1, /* min_channels_count; */
  4, /* max_channels_count; */
  1, /* min_colour_count; */
  4, /* max_colour_count; */
  0, /* can_planar; can read separated channels */
  1, /* can_interwoven; can read continuous channels */
  0, /* can_swap; can swap colour channels (BGR)*/
  0, /* can_swap_bytes; non host byte order */
  0, /* can_revert; revert 1 -> 0 and 0 -> 1 */
  1, /* can_premultiplied_alpha; */
  1, /* can_nonpremultiplied_alpha; */
  0, /* can_subpixel; understand subpixel order */
  0, /* oyCHANNELTYPE_e    * channel_types; */
  0, /* count in channel_types */
  2, /* id; relative to oyFilter_s, e.g. 1 */
  0  /* is_mandatory; mandatory flag */
};
oyConnectorImaging_s * oyra_imageOutputPPM_connectors_plug[2] = 
             { &oyra_imageOutputPPM_connector_in, 0 };

/**
 *  This function implements oyCMMGetText_f.
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/12/22 (Oyranos: 0.1.10)
 *  @date    2009/12/22
 */
const char * oyraApi4ImageWriteUiGetText (
                                       const char        * select,
                                       oyNAME_e            type,
                                       oyStruct_s        * context )
{
  static char * category = 0;
  if(strcmp(select,"name"))
  {
         if(type == oyNAME_NICK)
      return "write_ppm";
    else if(type == oyNAME_NAME)
      return _("Image[write_ppm]");
    else
      return _("Write PPM Image Filter Object");
  }
  else if(strcmp(select,"category"))
  {
    if(!category)
    {
      STRING_ADD( category, _("Files") );
      STRING_ADD( category, _("/") );
      STRING_ADD( category, _("Write PPM") );
    }
         if(type == oyNAME_NICK)
      return "category";
    else if(type == oyNAME_NAME)
      return category;
    else
      return category;
  }
  else if(strcmp(select,"help"))
  {
         if(type == oyNAME_NICK)
      return "help";
    else if(type == oyNAME_NAME)
      return _("Option \"filename\", a valid filename");
    else
      return _("The Option \"filename\" should contain a valid filename to write the ppm data into. A existing file will be overwritten without notice.");
  }
  return 0;
}
const char * oyra_api4_image_write_ppm_ui_texts[] = {"name", "category", "help", 0};

/** @instance oyra_api4_image_write_ppm_ui
 *  @brief    oyra oyCMMapi4_s::ui implementation
 *
 *  The UI for filter write ppm.
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/09/09 (Oyranos: 0.1.10)
 *  @date    2009/12/22
 */
oyCMMui_s oyra_api4_image_write_ppm_ui = {
  oyOBJECT_CMM_DATA_TYPES_S,           /**< oyOBJECT_e       type; */
  0,0,0,                            /* unused oyStruct_s fields; keep to zero */

  CMM_VERSION,                         /**< int32_t version[3] */
  {0,3,0},                            /**< int32_t module_api[3] */

  oyraFilter_ImageOutputPPMValidateOptions, /* oyCMMFilter_ValidateOptions_f */
  oyraWidgetEvent, /* oyWidgetEvent_f */

  "Files/Write PPM", /* category */
  ppm_write_extra_options, /* const char * options */
  oyraPPMwriteUiGet, /* oyCMMuiGet_f oyCMMuiGet */

  oyraApi4ImageWriteUiGetText, /* oyCMMGetText_f   getText */
  oyra_api4_image_write_ppm_ui_texts /* const char    ** texts */
};

/** @instance oyra_api4
 *  @brief    oyra oyCMMapi4_s implementation
 *
 *  A filter writing a PPM image.
 *
 *  @par Options:
 *  - "filename" - the file name to write to
 *
 *  @version Oyranos: 0.1.8
 *  @since   2008/10/07 (Oyranos: 0.1.8)
 *  @date    2008/10/07
 */
oyCMMapi4_s   oyra_api4_image_write_ppm = {

  oyOBJECT_CMM_API4_S, /* oyStruct_s::type oyOBJECT_CMM_API4_S */
  0,0,0, /* unused oyStruct_s fileds; keep to zero */
  (oyCMMapi_s*) & oyra_api7_image_write_ppm, /* oyCMMapi_s * next */
  
  oyraCMMInit, /* oyCMMInit_f */
  oyraCMMMessageFuncSet, /* oyCMMMessageFuncSet_f */

  /* registration */
  OY_TOP_SHARED OY_SLASH OY_DOMAIN_INTERNAL OY_SLASH OY_TYPE_STD "/write_ppm.file_write._CPU._" CMM_NICK,

  CMM_VERSION, /* int32_t version[3] */
  {0,3,0},                  /**< int32_t module_api[3] */
  0,   /* id_; keep empty */
  0,   /* api5_; keep empty */

  oyraFilterNode_ImageRootContextToMem, /* oyCMMFilterNode_ContextToMem_f */
  0, /* oyCMMFilterNode_GetText_f        oyCMMFilterNode_GetText */
  {0}, /* char context_type[8] */

  &oyra_api4_image_write_ppm_ui        /**< oyCMMui_s *ui */
};

/** @instance oyra_api7
 *  @brief    oyra oyCMMapi7_s implementation
 *
 *  A filter writing a PPM image.
 *
 *  @par Options:
 *  - "filename" - the file name to write to
 *
 *  @version Oyranos: 0.1.8
 *  @since   2008/10/07 (Oyranos: 0.1.8)
 *  @date    2008/10/07
 */
oyCMMapi7_s   oyra_api7_image_write_ppm = {

  oyOBJECT_CMM_API7_S, /* oyStruct_s::type oyOBJECT_CMM_API7_S */
  0,0,0, /* unused oyStruct_s fileds; keep to zero */
  (oyCMMapi_s*) & oyra_api4_image_input_ppm, /* oyCMMapi_s * next */
  
  oyraCMMInit, /* oyCMMInit_f */
  oyraCMMMessageFuncSet, /* oyCMMMessageFuncSet_f */

  /* registration */
  OY_TOP_SHARED OY_SLASH OY_DOMAIN_INTERNAL OY_SLASH OY_TYPE_STD "/write_ppm.file_write._CPU._" CMM_NICK,

  CMM_VERSION, /* int32_t version[3] */
  {0,3,0},                  /**< int32_t module_api[3] */
  0,   /* id_; keep empty */
  0,   /* api5_; keep empty */

  oyraFilterPlug_ImageOutputPPMWrite, /* oyCMMFilterPlug_Run_f */
  {0}, /* char data_type[8] */

  (oyConnector_s**) oyra_imageOutputPPM_connectors_plug,   /* plugs */
  1,   /* plugs_n */
  0,   /* plugs_last_add */
  (oyConnector_s**) oyra_imageOutputPPM_connectors_socket,   /* sockets */
  1,   /* sockets_n */
  0    /* sockets_last_add */
};


/* ---------------------------------------------------------------------------*/


oyOptions_s* oyraFilter_ImageInputPPMValidateOptions
                                     ( oyFilterCore_s    * filter,
                                       oyOptions_s       * validate,
                                       int                 statical,
                                       uint32_t          * result )
{
  uint32_t error = !filter;

  if(!error)
    error = !oyOptions_FindString( validate, "filename", 0 );

  *result = error;

  return 0;
}

int wread ( unsigned char* data, size_t pos, size_t max, size_t *start, size_t *end )
{
  int end_found = 0;

  if( max <= 1 ) return 0;

  while(pos < max && isspace( data[pos] )) ++pos;
  *start = pos;

  while(pos < max && !end_found) {
    if( isspace( data[pos] ) ) {
      end_found = 1;
      break;
    } else
      ++pos;
  }
  *end = pos;

  return end_found;
}

oyProfile_s * oyProfile_FromName     ( const char        * name )
{
    oyProfile_s * p = 0;
    char ** names = NULL;
    uint32_t count = 0, i;
    const char * t = 0;

    names = /*(const char**)*/ oyProfileListGet ( NULL, &count, malloc );

    if(name)
    {
      for(i = 0; i < (int)count; ++i)
      {
        p = oyProfile_FromFile( names[i], 0,0 );

        t = oyProfile_GetText(p, oyNAME_DESCRIPTION);
        if(t && strcmp(t,name) == 0)
        {
          free(names[i]);
          break;
        }
        free(names[i]);

        oyProfile_Release( &p );
      }
      free(names); names = 0;
    }
  return p;
}

/** @func    oyraFilterPlug_ImageInputPPMRun
 *  @brief   implement oyCMMFilter_GetNext_f()
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/02/18 (Oyranos: 0.1.10)
 *  @date    2009/02/18
 */
int      oyraFilterPlug_ImageInputPPMRun (
                                       oyFilterPlug_s    * requestor_plug,
                                       oyPixelAccess_s   * ticket )
{
  oyFilterSocket_s * socket = 0;
  oyFilterNode_s * node = 0;
  int error = 0;
  const char * filename = 0;
  FILE * fp = 0;
  oyDATATYPE_e data_type = oyUINT8;
  oyPROFILE_e profile_type = oyEDITING_RGB;
  oyProfile_s * prof = 0;
  oyImage_s * image_in = 0;
  oyPixel_t pixel_type = 0;
  int     fsize = 0;
  size_t  fpos = 0;
  uint8_t * data = 0, * buf = 0;
  size_t  mem_n = 0;   /* needed memory in bytes */
    
  int info_good = 1;

  int type = 0;        /* PNM type */
  int width = 0;
  int height = 0;
  int spp = 0;         /* samples per pixel */
  int byteps = 1;      /* byte per sample */
  double maxval = 0; 
    
  size_t start, end;

  /* passing through the data reading */
  if(requestor_plug->type_ == oyOBJECT_FILTER_PLUG_S &&
     requestor_plug->remote_socket_->data)
  {
    error = oyraFilterPlug_ImageRootRun( requestor_plug, ticket );

    return error;

  } else if(requestor_plug->type_ == oyOBJECT_FILTER_SOCKET_S)
  {
    /* To open the a image here seems not so straight forward.
     * Still the plug-in should be prepared to initialise the image data before
     * normal processing occurs.
     */
    socket = (oyFilterSocket_s*) requestor_plug;
    requestor_plug = 0;
    node = socket->node;

  } else {
    /* second option to open the file */
    socket = requestor_plug->remote_socket_;
    node = socket->node;
  }


  if(error <= 0)
    filename = oyOptions_FindString( node->core->options_, "filename", 0 );

  if(filename)
    fp = fopen( filename, "rm" );

  if(!fp)
  {
    oyra_msg( oyMSG_WARN, (oyStruct_s*)node,
             OY_DBG_FORMAT_ " could not open: %s",
             OY_DBG_ARGS_, oyNoEmptyString_m_( filename ) );
    return 1;
  }

  fseek(fp,0L,SEEK_END);
  fsize = ftell(fp);
  rewind(fp);

  oyAllocHelper_m_( data, uint8_t, fsize, 0, return 1);

  fpos = fread( data, sizeof(uint8_t), fsize, fp );
  if( fpos < fsize ) {
    oyra_msg( oyMSG_WARN, (oyStruct_s*)node,
             OY_DBG_FORMAT_ " could not read: %s %d %d",
             OY_DBG_ARGS_, oyNoEmptyString_m_( filename ), fsize, (int)fpos );
    oyFree_m_( data )
    fclose (fp);
    return FALSE;
  }

  fpos = 0;
  fclose (fp);
  fp = NULL;


  /* parse Infos */
  if(data[fpos] == 'P')
  {
    if(isdigit(data[++fpos])) {
      char tmp[2] = {0, 0};
      tmp[0] = data[fpos];
      type = atoi(tmp);
    } else
    if (!isspace(data[fpos]))
    {
      if(data[fpos] == 'F') /* PFM rgb */
        type = -6;
      else if (data[fpos] == 'f') /* PFM gray */
        type = -5;
      else
        info_good = 0;
    }
    else
      info_good = 0;
  }
  fpos++;

  /* parse variables */
  {
    int in_c = 0;    /* within comment */
    int v_read = 0;  /* number of variables allready read */
    int v_need = 3;  /* number of needed variable; start with three */
    int l_end = 0;   /* line end position */
    int l_pos = 0;   /* line position */
    int l_rdg = 1;   /* line reading */

    if(type == 1 || type == 4)
           v_need = 2;
    if(type == 7) /* pam  */
           v_need = 12;

    while(v_read < v_need && info_good)
    {
      l_pos = l_end = fpos;
      l_rdg = 1;

      /* read line */
      while(fpos < fsize && l_rdg)
      {
        if(data[fpos] == '#')
        {
          in_c = 1;
          l_end = fpos-1;
        } else if(data[fpos] == 10 || data[fpos] == 13) { /* line break */
          l_rdg = 0;
        } else if(data[fpos] != 0) {
          if(!in_c)
            ++l_end;
        } else {
          l_rdg = 0;
        }
        if(!l_rdg) {
          in_c = 0;
        }
        ++fpos;
      }

      /* lockup colour space */
      if(fpos - l_pos > 0)
      {
        if(fpos - l_pos >= 14 && memcmp(&data[l_pos],"# COLORSPACE: ", 14) == 0)
        { 
          char * t = oyAllocateFunc_(fpos - l_pos + 1);
          if(t)
          {
            memcpy( t, &data[l_pos+14], fpos - l_pos - 15 );
            t[fpos - l_pos - 15] = 0;
            prof = oyProfile_FromName(t);
            if(prof)
              fprintf(stderr, "found: %s\n", t);
            else
              oyra_msg( oyMSG_WARN, (oyStruct_s*)node,
             OY_DBG_FORMAT_ " could not find ICC: %s",
             OY_DBG_ARGS_, oyNoEmptyString_m_( t ) );
              
            oyDeAllocateFunc_(t);
          }
        }
      }

      /* parse line */
      while(info_good &&
            v_read < v_need &&
            l_pos < l_end)
      {
        if( info_good )
        {
          double var = -2;
          char var_s[64];
          int l = 0;
          wread ( data, l_pos, l_end, &start, &end );
          l = end - start;
          if ( l < 63 )
          {
            memcpy(var_s, &data[start], l);
            var_s[l] = 0;
            var = atof(var_s);
#           ifdef DEBUG_
            fprintf(stderr, "var = \"%s\"  %d\n",var_s, l);
#           endif
          }
          l_pos = end + 1;
          if(type == 7)
          {
            if(height == -1)
              height = (int)var;
            if(width == -1)
              width = (int)var;
            if(spp == -1)
              spp = (int)var;
            if(maxval == -0.5)
              maxval = var;

            if(strcmp(var_s, "HEIGHT") == 0)
              height = -1; /* expecting the next token is the val */
            if(strcmp(var_s, "WIDTH") == 0)
              width = -1;
            if(strcmp(var_s, "DEPTH") == 0)
              spp = -1;
            if(strcmp(var_s, "MAXVAL") == 0)
              maxval = -0.5;
            if(strcmp(var_s, "TUPLTYPE") == 0)
              ; /* ?? */
            if(strcmp(var_s, "ENDHDR") == 0)
              v_need = v_read;
          }
          else
          {
            if (!var)
              info_good = 0;
            if(v_read == 0)
              width = (int)var;
            else if(v_read == 1)
              height = (int)var;
            else if(v_read == 2)
              maxval = var;
          }

          ++v_read;

        }
      }
    }
  }

  if(strstr(strrchr(filename, '.')+1, "raw"))
  {
    info_good = 1;
    width = atoi(getenv("RAW_WIDTH"));
    height = atoi(getenv("RAW_HEIGHT"));
    type = atoi(getenv("RAW_TYPE"));
    fpos = 0;
    maxval = atoi(getenv("RAW_MAXVAL"));
  }


  if(info_good)
    switch(type) {
      case 1:
      case 4:
           data_type = oyUINT8;
           spp = 1;
           info_good = 0;
           break;
      case 2:
      case 5:
           if(maxval <= 255)
           {
             data_type = oyUINT8;
             byteps        = 1;
           } else if (maxval <= 65535) {
             data_type = oyUINT16;
             byteps        = 2;
           }
           spp = 1;
           break;
      case 3:
      case 6:
           if(maxval <= 255)
           {
             data_type = oyUINT8;
             byteps        = 1;
           } else if (maxval <= 65535) {
             data_type = oyUINT16;
             byteps        = 2;
           }
           spp = 3;
           break;
      case -5:
           data_type = oyFLOAT;
           byteps = 4;
           spp = 1;
           break;
      case -6:
           byteps = 4;
           spp = 3;
           data_type = oyFLOAT;
           break;
      case 7: /* pam */
           if (maxval == 1.0 || maxval == -1.0)
           {
             byteps        = 4;
             data_type = oyFLOAT;
           } else if(maxval <= 255) {
                     byteps        = 1;
             data_type = oyUINT8;
           } else if (maxval <= 65535) {
                     byteps        = 2;
             data_type = oyUINT16;
           }
           break;
      default:
           info_good = 0;
    }

    switch(spp)
    {
      case 1:
           profile_type = oyASSUMED_GRAY;
           break;
      case 2:
           profile_type = oyASSUMED_GRAY;
           break;
      case 3:
           profile_type = oyASSUMED_RGB;
           break;
      case 4:
           profile_type = oyASSUMED_RGB;
           break;
    }

  if( !info_good )
  {
    oyra_msg( oyMSG_WARN, (oyStruct_s*)node,
             OY_DBG_FORMAT_ "failed to get info of %s",
             OY_DBG_ARGS_, oyNoEmptyString_m_( filename ));
    oyFree_m_( data )
    return FALSE;
  }

  /* check if the file can hold the expected data (for raw only) */
  mem_n = width*height*byteps*spp;
  if(type == 5 || type == 6 || type == -5 || type == -6 || type == 7)
  {
    if (mem_n > fsize-fpos)
    {
      oyra_msg( oyMSG_WARN, (oyStruct_s*)node,
             OY_DBG_FORMAT_ "\n  storage size of %s is too small: %d",
             OY_DBG_ARGS_, oyNoEmptyString_m_( filename ),
             (int)mem_n-fsize-fpos );
      oyFree_m_( data )
      return FALSE;
    }

  } else
  {
    if (type == 2 || type == 3) {
      oyra_msg( oyMSG_WARN, (oyStruct_s*)node,
             OY_DBG_FORMAT_ "\n  %s contains ascii data, which are not handled by this pnm reader",
             OY_DBG_ARGS_, oyNoEmptyString_m_( filename ));
    } else if (type == 1 || type == 4) {
      oyra_msg( oyMSG_WARN, (oyStruct_s*)node,
             OY_DBG_FORMAT_ "\n  %s contains bitmap data, which are not handled by this pnm reader",
             OY_DBG_ARGS_, oyNoEmptyString_m_( filename ) );
    }
    oyFree_m_( data )
    return FALSE;
  }

  oyAllocHelper_m_( buf, uint8_t, mem_n, 0, return 1);

  /* the following code is almost completely taken from ku.b's ppm CP plug-in */
  {
    int h, j_h = 0, p, n_samples, n_bytes;
    int byte_swap = 0;
    unsigned char *d_8 = 0;
    unsigned char *src = &data[fpos];

    uint16_t *d_16;
    float  *d_f;

    if(oyBigEndian())
    {
      if(maxval < 0 && byteps == 4)
        byte_swap = 1;
    } else
    {
      if( (byteps == 2) ||
        (maxval > 0 && byteps == 4)  ) {
        byte_swap = 1;
      }
    }

    maxval = abs(maxval);

    for(h = 0; h < height; ++h)
    {
        n_samples = 1 * width * spp;
        n_bytes = n_samples * byteps;

        d_8  = buf;
        d_16 = (uint16_t*)buf;
        d_f  = (float*)buf;

        /*  TODO 1 bit raw and ascii */
        if (type == 1 || type == 4) {

        /*  TODO ascii  */
        } else if (type == 2 || type == 3) {


        /*  raw and floats */
        } else if (type == 5 || type == 6 ||
                   type == -5 || type == -6 ||
                   type == 7 )
        {
          if(byteps == 1) {
            d_8 = &src[ h * width * spp * byteps ];
          } else if(byteps == 2) {
            d_16 = (uint16_t*)& src[ h * width * spp * byteps ];
          } else if(byteps == 4) {
            d_f = (float*)&src[ h * width * spp * byteps ];
          }
          memcpy (&buf[ h * width * spp * byteps ],
                  &src[ (j_h + h) * width * spp * byteps ],
                  1 * width * spp * byteps);
        }

        /* normalise and byteswap */
        if( byte_swap )
        {
          unsigned char *c_buf = &buf[ h * width * spp * byteps ];
          char  tmp;
          if (byteps == 2) {         /* 16 bit */
#pragma omp parallel for private(tmp)
            for (p = 0; p < n_bytes; p += 2)
            {
              tmp = c_buf[p];
              c_buf[p] = c_buf[p+1];
              c_buf[p+1] = tmp;
            }
          } else if (byteps == 4) {  /* float */
#pragma omp parallel for private(tmp)
            for (p = 0; p < n_bytes; p += 4)
            {
              tmp = c_buf[p];
              c_buf[p] = c_buf[p+3];
              c_buf[p+3] = tmp;
              tmp = c_buf[p+1];
              c_buf[p+1] = c_buf[p+2];
              c_buf[p+2] = tmp;
            }
          }
        }

        if (byteps == 1 && maxval < 255) {         /*  8 bit */
#pragma omp parallel for
          for (p = 0; p < n_samples; ++p)
            d_8[p] = (d_8[p] * 255) / maxval;
        } else if (byteps == 2 && maxval < 65535) {/* 16 bit */
#pragma omp parallel for
          for (p = 0; p < n_samples; ++p)
            d_16 [p] = (d_16[p] * 65535) / maxval;
        } else if (byteps == 4 && maxval != 1.0) {  /* float */
#pragma omp parallel for
          for (p = 0; p < n_samples; ++p)
            d_f[p] = d_f[p] * maxval;
        }
    }
  }

  pixel_type = oyChannels_m(spp) | oyDataType_m(data_type); 
  if(!prof)
    prof = oyProfile_FromStd( profile_type, 0 );

  image_in = oyImage_Create( width, height, buf, pixel_type, prof, 0 );

  if (!image_in)
  {
      oyra_msg( oyMSG_WARN, (oyStruct_s*)node,
             OY_DBG_FORMAT_ "PNM can't create a new image\n%dx%d %d",
             OY_DBG_ARGS_,  width, height, pixel_type );
      oyFree_m_ (data)
    return FALSE;
  }

  error = oyOptions_SetFromText( &image_in->tags,
                                 "//" OY_TYPE_STD "/input_ppm.file_read"
                                                                    "/filename",
                                 filename, OY_CREATE_NEW );

  if(error <= 0)
  {
    socket->data = (oyStruct_s*)oyImage_Copy( image_in, 0 );
  }

  if(ticket &&
     ticket->output_image &&
     ticket->output_image->width == 0 &&
     ticket->output_image->height == 0)
  {
    ticket->output_image->width = image_in->width;
    ticket->output_image->height = image_in->height;
    oyImage_SetCritical( ticket->output_image, image_in->layout_[0], 0,0 );
  }

  oyImage_Release( &image_in );
  oyFree_m_ (data)

  /* return an error to cause the graph to retry */
  return 1;
}

const char ppm_read_extra_options[] = {
 "\n\
  <" OY_TOP_SHARED ">\n\
   <" OY_DOMAIN_INTERNAL ">\n\
    <" OY_TYPE_STD ">\n\
     <" "file_read" ">\n\
      <filename></filename>\n\
     </" "file_read" ">\n\
    </" OY_TYPE_STD ">\n\
   </" OY_DOMAIN_INTERNAL ">\n\
  </" OY_TOP_SHARED ">\n"
};

int  oyraPPMreadUiGet                ( oyOptions_s       * opts,
                                       char             ** xforms_layout,
                                       oyAlloc_f           allocateFunc )
{
  char * text = (char*)allocateFunc(5);
  text[0] = 0;
  *xforms_layout = text;
  return 0;
}


oyConnectorImaging_s oyra_imageInputPPM_connector = {
  oyOBJECT_CONNECTOR_IMAGING_S,0,0,
                               (oyObject_s)&oy_connector_imaging_static_object,
  oyCMMgetImageConnectorSocketText, /* getText */
  oy_image_connector_texts, /* texts */
  "//" OY_TYPE_STD "/image.data", /* connector_type */
  oyFilterSocket_MatchImagingPlug, /* filterSocket_MatchPlug */
  0, /* is_plug == oyFilterPlug_s */
  oyra_image_ppm_data_types,
  4, /* data_types_n; elements in data_types array */
  -1, /* max_colour_offset */
  1, /* min_channels_count; */
  4, /* max_channels_count; */
  1, /* min_colour_count; */
  4, /* max_colour_count; */
  0, /* can_planar; can read separated channels */
  1, /* can_interwoven; can read continuous channels */
  0, /* can_swap; can swap colour channels (BGR)*/
  0, /* can_swap_bytes; non host byte order */
  0, /* can_revert; revert 1 -> 0 and 0 -> 1 */
  1, /* can_premultiplied_alpha; */
  1, /* can_nonpremultiplied_alpha; */
  0, /* can_subpixel; understand subpixel order */
  0, /* oyCHANNELTYPE_e    * channel_types; */
  0, /* count in channel_types */
  1, /* id; relative to oyFilter_s, e.g. 1 */
  0  /* is_mandatory; mandatory flag */
};
oyConnectorImaging_s * oyra_imageInputPPM_connectors[2] = 
             { &oyra_imageInputPPM_connector, 0 };


/**
 *  This function implements oyCMMGetText_f.
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/12/22 (Oyranos: 0.1.10)
 *  @date    2009/12/22
 */
const char * oyraApi4ImageInputUiGetText (
                                       const char        * select,
                                       oyNAME_e            type,
                                       oyStruct_s        * context )
{
  static char * category = 0;
  if(strcmp(select,"name"))
  {
         if(type == oyNAME_NICK)
      return "input_ppm";
    else if(type == oyNAME_NAME)
      return _("Image[input_ppm]");
    else
      return _("Input PPM Image Filter Object");
  }
  else if(strcmp(select,"category"))
  {
    if(!category)
    {
      STRING_ADD( category, _("Files") );
      STRING_ADD( category, _("/") );
      STRING_ADD( category, _("Read PPM") );
    }
         if(type == oyNAME_NICK)
      return "category";
    else if(type == oyNAME_NAME)
      return category;
    else
      return category;
  }
  else if(strcmp(select,"help"))
  {
         if(type == oyNAME_NICK)
      return "help";
    else if(type == oyNAME_NAME)
      return _("Option \"filename\", a valid filename of a existing PPM image");
    else
      return _("The Option \"filename\" should contain a valid filename to read the ppm data from. If the file does not exist, a error will occure.\nThe oyEDITING_RGB ICC profile is attached.");
  }
  return 0;
}
const char * oyra_api4_image_input_ppm_ui_texts[] = {"name", "category", "help", 0};

/** @instance oyra_api4_ui_image_input_ppm
 *  @brief    oyra oyCMMapi4_s::ui implementation
 *
 *  The UI for filter input ppm.
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/09/09 (Oyranos: 0.1.10)
 *  @date    2009/09/09
 */
oyCMMui_s oyra_api4_ui_image_input_ppm = {
  oyOBJECT_CMM_DATA_TYPES_S,           /**< oyOBJECT_e       type; */
  0,0,0,                            /* unused oyStruct_s fields; keep to zero */

  CMM_VERSION,                         /**< int32_t version[3] */
  {0,3,0},                            /**< int32_t module_api[3] */

  oyraFilter_ImageInputPPMValidateOptions, /* oyCMMFilter_ValidateOptions_f */
  oyraWidgetEvent, /* oyWidgetEvent_f */

  "Files/Read PPM", /* category */
  ppm_read_extra_options, /* const char * options */
  oyraPPMreadUiGet, /* oyCMMuiGet_f oyCMMuiGet */

  oyraApi4ImageInputUiGetText, /* oyCMMGetText_f   getText */
  oyra_api4_image_input_ppm_ui_texts /* const char    ** texts */
};

/** @instance oyra_api4
 *  @brief    oyra oyCMMapi4_s implementation
 *
 *  A filter for reading a PPM image.
 *
 *  @par Options:
 *  - "filename" - the file name to read from
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/02/18 (Oyranos: 0.1.10)
 *  @date    2009/02/18
 */
oyCMMapi4_s   oyra_api4_image_input_ppm = {

  oyOBJECT_CMM_API4_S, /* oyStruct_s::type oyOBJECT_CMM_API4_S */
  0,0,0, /* unused oyStruct_s fileds; keep to zero */
  (oyCMMapi_s*) & oyra_api7_image_input_ppm, /* oyCMMapi_s * next */
  
  oyraCMMInit, /* oyCMMInit_f */
  oyraCMMMessageFuncSet, /* oyCMMMessageFuncSet_f */

  /* registration */
  OY_TOP_SHARED OY_SLASH OY_DOMAIN_INTERNAL OY_SLASH OY_TYPE_STD "/input_ppm.file_read._CPU._" CMM_NICK,

  CMM_VERSION, /* int32_t version[3] */
  {0,3,0},                  /**< int32_t module_api[3] */
  0,   /* id_; keep empty */
  0,   /* api5_; keep empty */

  oyraFilterNode_ImageRootContextToMem, /* oyCMMFilterNode_ContextToMem_f */
  0, /* oyCMMFilterNode_GetText_f        oyCMMFilterNode_GetText */
  {0}, /* char context_type[8] */

  &oyra_api4_ui_image_input_ppm        /**< oyCMMui_s *ui */
};

char * oyra_api7_image_input_ppm_properties[] =
{
  "file=read",    /* file load|write */
  "image=pixel",  /* image type, pixel/vector/font */
  "layers=1",     /* layer count, one for plain images */
  "icc=0",        /* image type ICC profile support */
  "ext=ppm,pnm,pbm,pgm,pfm", /* supported extensions */
  0
};

/** @instance oyra_api7
 *  @brief    oyra oyCMMapi7_s implementation
 *
 *  A filter reading a PPM image.
 *
 *  @par Options:
 *  - "filename" - the file name to read from
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/02/18 (Oyranos: 0.1.10)
 *  @date    2009/02/18
 */
oyCMMapi7_s   oyra_api7_image_input_ppm = {

  oyOBJECT_CMM_API7_S, /* oyStruct_s::type oyOBJECT_CMM_API7_S */
  0,0,0, /* unused oyStruct_s fileds; keep to zero */
  (oyCMMapi_s*) & oyra_api4_image_load, /* oyCMMapi_s * next */
  
  oyraCMMInit, /* oyCMMInit_f */
  oyraCMMMessageFuncSet, /* oyCMMMessageFuncSet_f */

  /* registration */
  OY_TOP_SHARED OY_SLASH OY_DOMAIN_INTERNAL OY_SLASH OY_TYPE_STD "/input_ppm.file_read._CPU._" CMM_NICK,

  CMM_VERSION, /* int32_t version[3] */
  {0,3,0},                  /**< int32_t module_api[3] */
  0,   /* id_; keep empty */
  0,   /* api5_; keep empty */

  oyraFilterPlug_ImageInputPPMRun, /* oyCMMFilterPlug_Run_f */
  {0}, /* char data_type[8] */

  0,   /* plugs */
  0,   /* plugs_n */
  0,   /* plugs_last_add */
  (oyConnector_s**) oyra_imageInputPPM_connectors,   /* sockets */
  1,   /* sockets_n */
  0,    /* sockets_last_add */

  oyra_api7_image_input_ppm_properties /* char * properties */
};




