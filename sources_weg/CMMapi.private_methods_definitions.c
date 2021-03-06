#include "oyCMMapi3_s_.h"
#include "oyCMMapi4_s_.h"
#include "oyCMMapi5_s_.h"
#include "oyCMMapi6_s_.h"
#include "oyCMMapi7_s_.h"
#include "oyCMMapi8_s_.h"
#include "oyCMMapi9_s_.h"
#include "oyCMMapi10_s_.h"

/** @internal
 *  @memberof oyCMMapi_s_
 *  @brief   check for completeness
 *
 *  @version Oyranos: 0.1.10
 *  @since   2007/12/06 (Oyranos: 0.1.8)
 *  @date    2009/09/02
 */
oyOBJECT_e       oyCMMapi_Check_     ( oyCMMapi_s_        * api )
{
  int error = !api;
  oyOBJECT_e type = oyOBJECT_NONE;
  int module_api = api->module_api[0]*10000 + api->module_api[1]*100
                    + api->module_api[2];

  if(api->type_ <= oyOBJECT_CMM_API_S ||
     api->type_ >= oyOBJECT_CMM_API_MAX)
    error = 1;
  else
  {
    if(module_api < 110 ||  /* last API break */
       OYRANOS_VERSION < module_api)
    {
      error = 1;
      WARNc2_S("Wrong API for: %s %s", oyStructTypeToText(api->type_),
               oyNoEmptyString_m_(api->registration));
      return type;
    }
  }

  if(error <= 0)
  switch(api->type_)
  {
    case oyOBJECT_CMM_API1_S:
    {
      /* This module type is obsolete and should be replaced by oyCMMapi4_s. */
      error = 1;
    } break;
    case oyOBJECT_CMM_API2_S:
    {
      /* This module type is obsolete and should be replaced by oyCMMapi8_s. */
      error = 1;
    } break;
    case oyOBJECT_CMM_API3_S:
    {
      oyCMMapi3_s_ * s = (oyCMMapi3_s_*)api;
      if(!(s->oyCMMInit &&
           s->oyCMMMessageFuncSet &&
           s->oyCMMProfileTag_GetValues &&
           /*s-> &&*/
           s->oyCMMProfileTag_Create ) )
        error = 1;
    } break;
    case oyOBJECT_CMM_API4_S:
    {
      oyCMMapi4_s_ * s = (oyCMMapi4_s_*)api;
      if(!(s->oyCMMInit &&
           s->oyCMMMessageFuncSet &&
           s->registration && s->registration[0] &&
           (s->version[0] || s->version[1] || s->version[2])))
      {
        error = 1;
        WARNc2_S("Incomplete module header: %s %s",
                 oyStructTypeToText(api->type_),
                 oyNoEmptyString_m_(api->registration));
      }
      if(!(s->oyCMMInit && s->ui))
      {
        int ui_module_api = s->ui->module_api[0]*10000
                            + s->ui->module_api[1]*100
                            + s->ui->module_api[2];

        if(ui_module_api < 110 ||  /* last API break */
          OYRANOS_VERSION < ui_module_api)
        {
          error = 1;
          WARNc2_S("Wrong UI API for: %s %s", oyStructTypeToText(api->type_),
                   oyNoEmptyString_m_(api->registration));
          return type;
        }
      }
      if(s->ui && s->ui->oyCMMFilter_ValidateOptions &&
         !s->ui->oyWidget_Event)
      {
        error = 1;
        WARNc2_S("Incomplete module UI function set: %s %s",
                 oyStructTypeToText(api->type_),
                 oyNoEmptyString_m_(api->registration));
      }
      if(s->context_type && s->context_type[0] &&
         !s->oyCMMFilterNode_ContextToMem)
      {
        error = 1;
        WARNc2_S("context_type provided but no oyCMMFilterNode_ContextToMem: %s %s",
                 oyStructTypeToText(api->type_),
                 oyNoEmptyString_m_(api->registration));
      }
      if(!(s->ui && s->ui->texts &&
           s->ui->getText && s->ui->getText("name", oyNAME_NAME)))
      {
        error = 1;
        WARNc2_S("Missed module name: %s %s",
                 oyStructTypeToText(api->type_),
                 oyNoEmptyString_m_(api->registration));
      }
      if(!(s->ui && s->ui->category && s->ui->category[0]))
      {
        error = 1;
        WARNc2_S("Missed module category: %s %s",
                 oyStructTypeToText(api->type_),
                 oyNoEmptyString_m_(api->registration));
      }
      if(s->ui && s->ui->options && s->ui->options[0] && !s->ui->oyCMMuiGet)
      {
        error = 1;
        WARNc2_S("options provided without oyCMMuiGet: %s %s",
                 oyStructTypeToText(api->type_),
                 oyNoEmptyString_m_(api->registration));
      }
    } break;
    case oyOBJECT_CMM_API5_S:
    {
      oyCMMapi5_s_ * s = (oyCMMapi5_s_*)api;
      if(!(s->oyCMMInit &&
           s->oyCMMMessageFuncSet &&
           s->registration && s->registration[0] &&
           (s->version[0] || s->version[1] || s->version[2]) &&
           s->oyCMMFilterLoad &&
           s->oyCMMFilterScan
            ) )
      {
        int module_api = 10000*s->module_api[0] + 100*s->module_api[1] + 1*s->module_api[2];
        if(module_api < 110) /* last API break */
          error = 1;
      }
    } break;
    case oyOBJECT_CMM_API6_S:
    {
      oyCMMapi6_s_ * s = (oyCMMapi6_s_*)api;
      if(!(s->oyCMMInit &&
           s->oyCMMMessageFuncSet &&
           s->registration && s->registration[0] &&
           (s->version[0] || s->version[1] || s->version[2]) &&
           s->data_type_in && s->data_type_in[0] &&
           s->data_type_out && s->data_type_out[0] &&
           s->oyCMMdata_Convert
            ) )
        error = 1;
    } break;
    case oyOBJECT_CMM_API7_S:
    {
      oyCMMapi7_s_ * s = (oyCMMapi7_s_*)api;
      if(!(s->oyCMMInit &&
           s->oyCMMMessageFuncSet &&
           s->registration && s->registration[0] &&
           (s->version[0] || s->version[1] || s->version[2]) &&
           s->oyCMMFilterPlug_Run &&
           ((s->plugs && s->plugs_n) || (s->sockets && s->sockets_n))
            ) )
      {
        int module_api = 10000*s->module_api[0] + 100*s->module_api[1] + 1*s->module_api[2];
        if(module_api < 110) /* last API break */
          error = 1;
      }
    } break;
    case oyOBJECT_CMM_API8_S:
    {
      oyCMMapi8_s_ * s = (oyCMMapi8_s_*)api;
      if(!(s->oyCMMInit &&
           s->oyCMMMessageFuncSet &&
           s->registration && s->registration[0] &&
           (s->version[0] || s->version[1] || s->version[2]) &&
           s->oyConfigs_FromPattern &&
           s->oyConfigs_Modify &&
           s->oyConfig_Rank &&
           s->rank_map
            ) )
        error = 1;
    } break;
    case oyOBJECT_CMM_API9_S:
    {
      oyCMMapi9_s_ * s = (oyCMMapi9_s_*)api;
      if(!(s->oyCMMInit &&
           s->oyCMMMessageFuncSet &&
           s->registration && s->registration[0] &&
           (s->version[0] || s->version[1] || s->version[2]) &&
           (!s->options ||
            (s->options && s->oyCMMFilter_ValidateOptions &&
             s->oyCMMuiGet && s->oyWidget_Event))&&
           (!s->texts ||
            ((s->texts || s->getText)
              && s->texts[0] && s->texts[0][0] && s->getText)) &&
           s->pattern && s->pattern[0]
            ) )
      {
        int module_api = 10000*s->module_api[0] + 100*s->module_api[1] + 1*s->module_api[2];
        if(module_api < 110) /* last API break */
          error = 1;
      }
    } break;
    case oyOBJECT_CMM_API10_S:
    {
      oyCMMapi10_s_ * s = (oyCMMapi10_s_*)api;
      if(!(s->oyCMMInit &&
           s->oyCMMMessageFuncSet &&
           s->registration && s->registration[0] &&
           (s->version[0] || s->version[1] || s->version[2]) &&
           (!s->texts ||
            ((s->texts || s->getText)
              && s->texts[0] && s->texts[0][0] && s->getText)) &&
           s->oyMOptions_Handle
            ) )
        error = 1;
    } break;
    default: break;
  }

  if(error <= 0)
    type = api->type_;
  else
    WARNc2_S("Found problems with: %s %s", oyStructTypeToText(api->type_),
              oyNoEmptyString_m_(api->registration));


  return type;
}

/** @internal
 *  Function oyCMMapi_CheckWrap_
 *  @memberof oyCMMapi_s_
 *
 *  @version Oyranos: 0.1.10
 *  @since   2007/12/16 (Oyranos: 0.1.9)
 *  @date    2009/09/02
 */
oyOBJECT_e   oyCMMapi_CheckWrap_     ( oyCMMInfo_s_      * cmm_info,
                                       oyCMMapi_s_       * api,
                                       oyPointer           data,
                                       uint32_t          * rank )
{
  oyOBJECT_e type = oyCMMapi_Check_( api );
  if(rank)
  {
    if(type)
      *rank = 1;
    else
      *rank = 0;
  }
  return type;
}

/** @internal
 *  Function oyCMMapi_selectFilter_
 *  @brief   filter the desired api
 *  @memberof oyCMMapi_s_
 *
 *  @version Oyranos: 0.1.9
 *  @since   2008/12/16 (Oyranos: 0.1.9)
 *  @date    2008/12/16
 */
oyOBJECT_e   oyCMMapi_selectFilter_  ( oyCMMInfo_s_      * info,
                                       oyCMMapi_s_       * api,
                                       oyPointer           data,
                                       uint32_t          * rank )
{
  oyOBJECT_e type = oyOBJECT_NONE;
  oyCMMapiFilter_s_ * cmm_api = (oyCMMapiFilter_s *) api;
  int error = !data || !api;
  oyRegistrationData_s * reg_filter;
  int found = 0;

  if(error <= 0)
    reg_filter = (oyRegistrationData_s*) data;

  if(error <= 0 &&
     api->type_ == reg_filter->type_)
  {
    if(reg_filter->registration)
    {
      if(oyFilterRegistrationMatch( cmm_api->registration,
                                    reg_filter->registration, api->type_ ))
        found = 1;
    } else
      found = 1;

    if( found )
    {
      type = api->type_;
      if(rank)
        *rank = 1;
      else
        *rank = 0;
    }
  }

  return type;
}
