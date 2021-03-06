/** @file oyPointer_s_.h

   [Template file inheritance graph]
   +-> Pointer_s_.template.h
   |
   +-- Base_s_.h

 *  Oyranos is an open source Colour Management System
 *
 *  @par Copyright:
 *            2004-2012 (C) Kai-Uwe Behrmann
 *
 *  @author   Kai-Uwe Behrmann <ku.b@gmx.de>
 *  @par License:
 *            new BSD - see: http://www.opensource.org/licenses/bsd-license.php
 *  @date     2012/02/14
 */


#ifndef OY_POINTER_S__H
#define OY_POINTER_S__H

#define oyPointerPriv_m( var ) ((oyPointer_s_*) (var))

  
#include <oyranos_object.h>
  

#include "oyPointer_s.h"

typedef struct oyPointer_s_ oyPointer_s_;
/* Include "Pointer.private.h" { */

/* } Include "Pointer.private.h" */

/** @internal
 *  @struct   oyPointer_s_
 *  @brief      custom pointer
 *  @ingroup  objects_generic
 *  @extends  oyStruct_s
 */
struct oyPointer_s_ {

/* Include "Struct.members.h" { */
oyOBJECT_e           type_;          /**< @private struct type */
oyStruct_Copy_f      copy;           /**< copy function */
oyStruct_Release_f   release;        /**< release function */
oyObject_s           oy_;            /**< @private features name and hash */

/* } Include "Struct.members.h" */



/* Include "Pointer.members.h" { */
  char               * lib_name;       /*!< the CMM */
  char               * func_name;      /*!< optional the CMM's function name */
  oyPointer            ptr;            /*!< a CMM's data pointer */
  int                  size;           /**< -1 && 0 - unknown, > 1 size of ptr*/
  char               * resource;       /**< the resource type, e.g. oyCMM_PROFILE, oyCMM_COLOUR_CONVERSION */
  oyPointer_release_f  ptrRelease;     /*!< CMM's deallocation function */
  int                  ref;            /**< Oyranos reference counter */

/* } Include "Pointer.members.h" */

};


oyPointer_s_*
  oyPointer_New_( oyObject_s object );
oyPointer_s_*
  oyPointer_Copy_( oyPointer_s_ *pointer, oyObject_s object);
oyPointer_s_*
  oyPointer_Copy__( oyPointer_s_ *pointer, oyObject_s object);
int
  oyPointer_Release_( oyPointer_s_ **pointer );



/* Include "Pointer.private_methods_declarations.h" { */
int                oyPointer_Set_    ( oyPointer_s_      * cmm_ptr,
                                       const char        * lib_name,
                                       const char        * resource,
                                       oyPointer           ptr,
                                       const char        * func_name,
                                       oyPointer_release_f ptrRelease );

/* } Include "Pointer.private_methods_declarations.h" */



void oyPointer_Release__Members( oyPointer_s_ * pointer );
int oyPointer_Init__Members( oyPointer_s_ * pointer );
int oyPointer_Copy__Members( oyPointer_s_ * dst, oyPointer_s_ * src);


#endif /* OY_POINTER_S__H */
