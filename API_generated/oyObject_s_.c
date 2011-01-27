/** @file oyObject_s_.c

   [Template file inheritance graph]
   +-- Object_s_.template.c

 *  Oyranos is an open source Colour Management System
 *
 *  @par Copyright:
 *            2004-2011 (C) Kai-Uwe Behrmann
 *
 *  @author   Kai-Uwe Behrmann <ku.b@gmx.de>
 *  @par License:
 *            new BSD - see: http://www.opensource.org/licenses/bsd-license.php
 *  @since    2011/01/26
 */


#include <math.h>

#include <oyranos_object_internal.h>

#include "oyObject_s.h"
#include "oyObject_s_.h"


/* Include "Object.private_methods_definitions.c" { */
/** @internal
 *  @memberof oyObject_s
 *  @brief   custom object memory managers
 *
 *  @param[in]    object         the object to modify
 *  @param[in]    allocateFunc   zero for default or user memory allocator
 *  @param[in]    deallocateFunc zero for default or user memory deallocator
 *
 *  @since Oyranos: version 0.1.8
 *  @date  november 2007 (API 0.1.8)
 */
oyObject_s   oyObject_SetAllocators_  ( oyObject_s        object,
                                        oyAlloc_f         allocateFunc,
                                        oyDeAlloc_f       deallocateFunc )
{
  if(!object) return 0;

  /* we require a allocation function to be present */
  if(allocateFunc)
    object->allocateFunc_ = allocateFunc;
  else
    object->allocateFunc_ = oyAllocateFunc_;

  object->deallocateFunc_ = deallocateFunc;

  return object;
}

/**
 *  @internal
 *  Function oyObject_Ref
 *  @memberof oyObject_s
 *  @brief   increase the ref counter and return the above zero ref value
 *
 *  @version Oyranos: 0.1.8
 *  @date    2008/02/07
 *  @since   2008/02/07 (Oyranos: 0.1.8)
 */
int          oyObject_Ref            ( oyObject_s          obj )
{
  oyObject_s s = obj;
  int error = !s;

  if( s->type_ != oyOBJECT_OBJECT_S)
  {
    WARNc2_S("Attempt to manipulate a non oyObject_s object; type: %d ID: %d",
             s->type_, s->id_)
    return 1;
  }

  if(error <= 0)
    oyObject_Lock( s, __FILE__, __LINE__ );

  if(error <= 0)
  {
    ++s->ref_;
  }
#   if DEBUG_OBJECT
    WARNc3_S("%s   ID: %d refs: %d",
             oyStructTypeToText( s->parent_type_ ), s->id_, s->ref_)
#   endif

  if(obj->parent_type_ == oyOBJECT_NAMED_COLOURS_S)
  {
    int e_a = error;
    error = pow(e_a,2.1);
    error = e_a;
  }

  if(error <= 0)
    oyObject_UnLock( s, __FILE__, __LINE__ );

  return s->ref_;
}

/** @internal
 *  Function oyObject_Hashed_
 *  @memberof oyObject_s
 *  @brief   check if a object has a hash sum computed
 *
 *  @version Oyranos: 0.1.8
 *  @date    2008/11/02
 *  @since   2008/11/02 (Oyranos: 0.1.8)
 */
int32_t      oyObject_Hashed_        ( oyObject_s          s )
{
  int32_t hashed = 0;
  if(s && s->type_ == oyOBJECT_OBJECT_S && s->hash_ptr_)
      if(((uint32_t*)(&s->hash_ptr_[0])) ||
         ((uint32_t*)(&s->hash_ptr_[4])) ||
         ((uint32_t*)(&s->hash_ptr_[8])) ||
         ((uint32_t*)(&s->hash_ptr_[12])) )
        hashed = 1;
  return hashed;
}

/** @internal
 *  Function oyObject_HashSet
 *  @memberof oyObject_s
 *  @brief   check if a object has a hash allocated and set it
 *
 *  @param[in,out] s                   the object
 *  @param[in]     hash                the hash of max size 2 * OY_HASH_SIZE
 *  @return                            0 - fine, otherwise error
 *
 *  @version Oyranos: 0.1.10
 *  @date    2009/04/16
 *  @since   2009/04/16 (Oyranos: 0.1.10)
 */
int          oyObject_HashSet          ( oyObject_s        s,
                                         const unsigned char * hash )
{
  int error = 0;

  if(s && s->type_ == oyOBJECT_OBJECT_S && !s->hash_ptr_)
  {
    s->hash_ptr_ = s->allocateFunc_(OY_HASH_SIZE*2);
    error = !s->hash_ptr_;
  }

  if(!error)
  {
    if(hash)
      memcpy( s->hash_ptr_, hash, OY_HASH_SIZE*2 );
    else
      memset( s->hash_ptr_, 0, OY_HASH_SIZE*2 );
  }

  return error;
}

/** @internal
 *  Function oyObject_HashEqual
 *  @memberof oyObject_s
 *  @brief   check if two objects hash is equal
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/04/16 (Oyranos: 0.1.10)
 *  @date    2009/05/26
 */
int          oyObject_HashEqual        ( oyObject_s        s1,
                                         oyObject_s        s2 )
{
  int equal = 0;

  if(s1->hash_ptr_ &&
     s2->hash_ptr_)
  {
    if(memcmp(s1->hash_ptr_, s2->hash_ptr_, OY_HASH_SIZE*2) == 0)
      return 1;
  }

  return equal;
}

/* } Include "Object.private_methods_definitions.c" */
