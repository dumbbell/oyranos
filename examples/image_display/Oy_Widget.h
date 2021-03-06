/**
 *  Oyranos is an open source Colour Management System 
 * 
 *  @par Copyright:
 *            2010 (C) Kai-Uwe Behrmann
 *
 *  @author   Kai-Uwe Behrmann <ku.b@gmx.de>
 *  @par License:
 *            new BSD <http://www.opensource.org/licenses/bsd-license.php>
 *  @since    2010/09/10
 *
 *  Helpers to create colour managed widgets with Oyranos.
 */

#ifndef Oy_Widget_H
#define Oy_Widget_H

#include <oyranos.h>
#include <oyranos_alpha.h>

#ifdef USE_GETTEXT
#include "config.h" /* I18N */
#include "fl_i18n/fl_i18n.H"
#else
#define _(text) text
#endif

class Oy_Widget
{
  oyConversion_s * context;

public:
  oyRectangle_s * old_display_rectangle;
  oyRectangle_s * old_roi_rectangle;
  int dirty;

  Oy_Widget()
  {
    context = 0;
    old_display_rectangle = oyRectangle_NewWith( 0,0,0,0, 0 );
    old_roi_rectangle = oyRectangle_NewWith( 0,0,0,0, 0 );
    dirty = 0;
  };

  ~Oy_Widget(void)
  {
    oyConversion_Release( &context );
    oyRectangle_Release( &old_display_rectangle );
    oyRectangle_Release( &old_roi_rectangle );
  };

  void conversion( oyConversion_s * c ) 
  {
    oyConversion_Release( &context );
    context = oyConversion_Copy( c, 0 );
  }

  oyConversion_s * conversion() { return context; }

  oyFilterNode_s * setImageType( const char * file_name, oyDATATYPE_e data_type, const char * cc_name, oyOptions_s * cc_options )
  {
    oyFilterNode_s * icc = 0;
    oyConversion_s * c = oyConversion_FromImageFileNameForDisplay( 
                             file_name, &icc, oyOPTIONATTRIBUTE_ADVANCED,
                             data_type, cc_name, cc_options, 0 );

    conversion( c );

    oyConversion_Release( &c );

    return icc;
  }
  oyFilterNode_s * setImageType( oyImage_s * image, oyImage_s * display_image, oyDATATYPE_e data_type, const char * cc_name, oyOptions_s * cc_options )
  {
    oyFilterNode_s * icc = 0;
    oyConversion_s * c = oyConversion_FromImageForDisplay( 
                             image, display_image, &icc, oyOPTIONATTRIBUTE_ADVANCED,
                             data_type, cc_name, cc_options, 0 );

    conversion( c );

    oyConversion_Release( &c );

    return icc;
  }

  void observeICC(                     oyFilterNode_s * icc,
                     int(*observator)( oyObserver_s      * observer,
                                       oySIGNAL_e          signal_type,
                                       oyStruct_s        * signal_data ) )
  {
    /* observe the icc node */
    oyBlob_s * b = oyBlob_New(0);
    oyBlob_SetFromStatic( b, this, 0, 0 );
    oyStruct_ObserverAdd( (oyStruct_s*)icc, (oyStruct_s*)conversion(),
                          (oyStruct_s*)b,
                          observator );
    oyBlob_Release( &b );
  }
};


#endif /* Oy_Widget_H */
