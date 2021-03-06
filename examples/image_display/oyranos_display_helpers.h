/**
 *  Oyranos is an open source Colour Management System 
 * 
 *  @par Copyright:
 *            2009-2012 (C) Kai-Uwe Behrmann
 *
 *  @author   Kai-Uwe Behrmann <ku.b@gmx.de>
 *  @par License:
 *            new BSD <http://www.opensource.org/licenses/bsd-license.php>
 *  @since    2010/09/05
 *
 *  Oyranos helpers for handling on screen display of images.
 */

#ifndef oyranos_display_helpers_h
#define oyranos_display_helpers_h

#include <oyranos.h>
#include <alpha/oyranos_alpha.h>

extern int oy_display_verbose;

oyConversion_s * oyConversion_FromImageForDisplay  (
                                       oyImage_s         * image_in,
                                       oyImage_s         * image_out,
                                       oyFilterNode_s   ** icc_node,
                                       uint32_t            flags,
                                       oyDATATYPE_e        data_type,
                                       const char        * cc_name,
                                       oyOptions_s       * cc_options,
                                       oyObject_s          obj );
oyConversion_s * oyConversion_FromImageFileNameForDisplay  (
                                       const char        * file_name,
                                       oyFilterNode_s   ** icc_node,
                                       uint32_t            flags,
                                       oyDATATYPE_e        data_type,
                                       const char        * cc_name,
                                       oyOptions_s       * cc_options,
                                       oyObject_s          obj );
int  oyDrawScreenImage               ( oyConversion_s    * context,
                                       oyPixelAccess_s   * ticket,
                                       oyRectangle_s     * display_rectangle,
                                       oyRectangle_s     * old_display_rectangle,
                                       oyRectangle_s     * old_roi_rectangle,
                                       const char        * system_type,
                                       oyDATATYPE_e        data_type_request,
                                       void              * display,
                                       void              * window,
                                       int                 dirty,
                                       oyImage_s         * image );
oyProfile_s * getEditingProfile      ( );

#ifdef HAVE_X
#include <X11/Xlib.h>
#endif

#ifdef USE_GETTEXT
#include "config.h" /* I18N */
#include "fl_i18n/fl_i18n.H"
#else
#define _(text) text
#endif


extern "C" {
const char *   oyDatatypeToText      ( oyDATATYPE_e        t); }

#ifndef OY_MIN
#define OY_MIN(a,b)    (((a) <= (b)) ? (a) : (b))
#endif


#endif /* oyranos_display_helpers_h */
