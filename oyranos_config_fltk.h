// generated by Fast Light User Interface Designer (fluid) version 1.0107

#ifndef oyranos_config_fltk_h
#define oyranos_config_fltk_h
#include <FL/Fl.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Scroll.H>
#include <FL/Flmm_Tabs.H>
#include <FL/fl_ask.H>
#include <FL/fl_draw.H>
#include "oyranos.h"
#include "oyranos_debug.h"
#include "oyranos_config.h"
#include "oyranos_helper.h"
#include "oyranos_alpha.h"
#include "config.h"
#include "fl_i18n/fl_i18n.H"
#include <iostream>
#include <fstream>
#include <cstdio>
using namespace oyranos;
#define H_SPACING 10
#define V_SPACING 3
#define BUTTON_HEIGHT 25
#define BOX_WIDTH 250
#define SELECT_WIDTH 176
void selectDefaultProfile_callback( Fl_Widget* w, void* );
void showDefaultProfile_callback( Fl_Widget* w, void* );
void addPathCallback( Fl_Widget* w, void* );
void rmPathCallback( Fl_Widget* w, void* );
void selectBehaviourCallback( Fl_Widget* w, void* x );
void debug_me( );

class OyFl_Pack : public Fl_Pack {
public:
  OyFl_Pack(int x, int y, int w, int h , const char *t = 0) ;
private:
  void draw();
};

class OyFl_Box : public Fl_Box {
public:
  OyFl_Box(int x, int y, int w, int h , const char *t = 0) ;
private:
  void draw();
  char *label_orig;
public:
  const char * label();
  void label(const char * l);
  void copy_label(const char* l);
};

class Option : public OyFl_Pack {
public:
  OyFl_Box   *box;
  Fl_Choice*choice;

  oyWIDGET_e option;
  oyWIDGET_TYPE_e type;
  int       i;
  Option( int x, int y, int w, int h, const char *name,
  oyWIDGET_e option_,
  oyWIDGET_TYPE_e type_,
  int choices_n,
  const char **choices,
  int current,
  const char *tooltip,
  int flags) ;
};

class ListEntry : public OyFl_Pack {
public:
  Fl_Box   *box;
  Fl_Button*button_add, *button_remove;
  int flags;
  ListEntry( int x, int y, int w, int h, const char * name, int flags )
    ;
};
void createUI();
void updateUI();
void updateUIIdle(void*);
#include <FL/Fl_Double_Window.H>
extern Fl_Double_Window *top_group;
#include <FL/Fl_Box.H>
extern Flmm_Tabs *top_tabs;
#include <FL/Fl_Group.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Pack.H>
extern Fl_Box *policy_box;
void listWindow( Fl_Widget *w );
#endif
