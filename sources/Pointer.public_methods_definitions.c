/** Function oyPointer_Set
 *  @brief   set a oyPointer_s
 *  @ingroup module_api
 *  @memberof oyPointer_s
 *
 *  Use for initialising.
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/07/22 (Oyranos: 0.1.10)
 *  @date    2009/08/16
 */
int          oyPointer_Set           ( oyPointer_s       * cmm_ptr,
                                       const char        * lib_name,
                                       const char        * resource,
                                       oyPointer           ptr,
                                       const char        * func_name,
                                       oyPointer_release_f ptrRelease )
{
  return oyPointer_Set_( (oyPointer_s_*)cmm_ptr, lib_name, resource, ptr, func_name, ptrRelease);
}

const char * oyPointer_GetFuncName    ( oyPointer_s        * cmm_ptr )
{
  oyPointer_s_ * c = (oyPointer_s_ *) cmm_ptr;
  if(c)
    return c->func_name;
  else
    return NULL;
}
const char * oyPointer_GetLibName     ( oyPointer_s        * cmm_ptr )
{
  oyPointer_s_ * c = (oyPointer_s_ *) cmm_ptr;
  if(c)
    return c->lib_name;
  else
    return NULL;
}
const char * oyPointer_GetResourceName( oyPointer_s        * cmm_ptr )
{
  oyPointer_s_ * c = (oyPointer_s_ *) cmm_ptr;
  if(c)
    return c->resource;
  else
    return NULL;
}
int          oyPointer_GetSize        ( oyPointer_s        * cmm_ptr )
{
  oyPointer_s_ * c = (oyPointer_s_ *) cmm_ptr;
  if(c)
    return c->size;
  else
    return 0;
}
int          oyPointer_SetSize        ( oyPointer_s        * cmm_ptr,
                                       int                 size )
{
  oyPointer_s_ * c = (oyPointer_s_ *) cmm_ptr;
  if(c)
    c->size = size;
  return 0;
}
oyPointer    oyPointer_GetPointer     ( oyPointer_s        * cmm_ptr )
{
  oyPointer_s_ * c = (oyPointer_s_ *) cmm_ptr;
  if(c)
    return c->ptr;
  else
    return NULL;
}

