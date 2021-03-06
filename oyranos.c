/** @file oyranos.c
 *
 *  Oyranos is an open source Colour Management System 
 *
 *  @par Copyright:
 *            2004-2011 (C) Kai-Uwe Behrmann
 *
 *  @brief    public Oyranos API's
 *  @author   Kai-Uwe Behrmann <ku.b@gmx.de>
 *  @par License:
 *            new BSD <http://www.opensource.org/licenses/bsd-license.php>
 *  @since    2004/11/25
 */


#include "oyranos.h" /* define HAVE_POSIX */

#include <sys/stat.h>
#ifdef HAVE_POSIX
#include <unistd.h>
#include <langinfo.h>
#endif
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "oyranos_check.h"
#include "oyranos_debug.h"
#include "oyranos_elektra.h"
#include "oyranos_helper.h"
#include "oyranos_internal.h"
#include "oyranos_io.h"
#include "oyranos_sentinel.h"
#include "oyranos_string.h"
#include "oyranos_texts.h"
#include "oyranos_xml.h"

#ifdef __APPLE__
#include "oyranos_cs.h"
#endif

/* --- Helpers  --- */
#if 1
#define ERR if (rc<=0 && oy_debug) { oyMessageFunc_p(oyMSG_WARN,0,"%s:%d %d\n", __FILE__,__LINE__,rc); perror("Error"); }
#else
#define ERR
#endif

#define WARNc_PROFILE_S(text_,fileName_) \
      { \
        int l_1 = oyStrlen_(text_); \
        int l_2 = oyStrlen_(oyNoEmptyName_m_(fileName_)); \
        char * tmp = oyAllocateFunc_ (l_1 + l_2 + 12); \
        memcpy(tmp, text_, l_1); \
        memcpy(&tmp[l_1], " ", 1); \
        memcpy(&tmp[l_1+1], oyNoEmptyName_m_(fileName_), l_2); \
        tmp[l_1+1+l_2] = 0; \
 \
        WARNc_S( tmp ); \
        oyDeAllocateFunc_(tmp); \
      }

/* --- static variables   --- */

/* --- structs, typedefs, enums --- */

/* --- internal API definition --- */

/* --- function definitions --- */




int
oyGetPathFromProfileNameCb_          ( oyFileList_s      * data,
                                       const char        * full_name,
                                       const char        * filename )
{
  int success = 0;
  oyFileList_s * l = data;
  char * search = l->names[0];
  const char * name = 0;
  int l1 = strlen(full_name),
      l2 = strlen(search),
      len = 0;

  if(l1 > l2)
  {
    len = l1 - l2;
    if(full_name[len-1] == OY_SLASH_C)
      name = &full_name[len];
    else
      name = filename;
  }

  DBG_MEM_S( search )
  if(name && l2 && strcmp( search, name ) == 0)
  {
    size_t size = 128;
    char* header = oyReadFileToMem_ (full_name, &size, oyAllocateFunc_);
    success = !oyCheckProfileMem_ (header, size, 0);
    oyFree_m_ (header);
    if (success) {
      DBG_MEM_S(full_name)
      DBG_MEM_V(oyStrlen_(full_name))
      if (oyStrlen_(full_name) < MAX_PATH) {
        oySprintf_( search, "%s", full_name );
        search[oyStrlen_(full_name)] = '\000';
      } else
        search[0] = '\000';
    } else
      WARNc_PROFILE_S( _("not a profile:"), oyNoEmptyName_m_(full_name) )
  }
  /* break on success */
  DBG_MEM_V(success)
  return success;
}

char*
oyGetPathFromProfileName_       (const char*   fileName,
                                 oyAlloc_f     allocate_func)
{
  char  *fullFileName = 0;
  char  *pathName = 0;
  int    success = 0;
  char  *header = 0;
  size_t    size;

  DBG_PROG_START

  if(fileName && fileName[0] && strlen(fileName) > 7 &&
     memcmp(fileName, "file://", 7) == 0)
    fileName = &fileName[7];

  /*DBG_NUM_S((fileName)) */
  /* search in configured paths */
  if (fileName && fileName[0] != OY_SLASH_C)
  {
    char search[MAX_PATH];
    int count = 0;
    size_t len = (oyStrlen_(fileName) < MAX_PATH) ? 
                          oyStrlen_(fileName) : MAX_PATH;
    char ** path_names = oyProfilePathsGet_( &count, oyAllocateFunc_ );
    char * l_names[2] = { 0, 0 };
    oyFileList_s l = {oyOBJECT_FILE_LIST_S_, 0, NULL, 0, 0, 0};

    l_names[0] = search;
    l.names = l_names;

    DBG_PROG

    if(oyStrlen_(fileName) < MAX_PATH)
    {
      memcpy(search, fileName, len); search[len] = '\000';
    } else {
      WARNc2_S( "%s %d", _("name longer than"), MAX_PATH)
      DBG_PROG_ENDE
      return 0;
    }
    success = oyRecursivePaths_( oyGetPathFromProfileNameCb_, &l,
                                 (const char**)path_names, count );

    oyStringListRelease_( &path_names, count, oyDeAllocateFunc_ );

      if (success) { /* found */
        len = 0;
        DBG_PROG_S((search))
        if(search[0] != 0) len = oyStrlen_(search);
        if(len) {
          char *ptr = 0;
          oyAllocHelper_m_( pathName, char, len+1, allocate_func, return 0 );
          oyStrcpy_(pathName, search);
          ptr = oyStrrchr_(pathName , OY_SLASH_C);
          if(ptr)
            ptr[0] = '\000';
        }
        DBG_PROG_S( pathName )
        DBG_PROG_ENDE
        return pathName;
      } else

    if (!success && !strchr(fileName, OY_SLASH_C)) {
      if(oy_warn_)
        WARNc_PROFILE_S(_("profile not found in colour path:"), fileName);
      DBG_PROG_ENDE
      return 0;
    }
  }

  if(fileName && !success)
  {/* use fileName as an full qualified name, check name and test profile*/
    DBG_PROG_S("dir/filename found")
    fullFileName = oyMakeFullFileDirName_ (fileName);

    if (oyIsFileFull_(fullFileName,"rb"))
    {
      size = 128;
      header = oyReadFileToMem_ (fullFileName, &size, allocate_func);

      if (size >= 128)
        success = !oyCheckProfileMem_ (header, 128, 0);
    }

    if (!success) {
      WARNc_PROFILE_S( _("profile not found:"), oyNoEmptyName_m_(fileName))
      DBG_PROG_ENDE
      return 0;
    }

    pathName = oyExtractPathFromFileName_(fullFileName);

    oyFree_m_ (header);
  }

  if (!success)
  { oyFree_m_ (pathName);
    pathName = 0;
  }

  oyFree_m_ (fullFileName);

  DBG_PROG_ENDE
  return pathName;
}


/* public API implementation */

/* path names API */

/* default profiles API */
int
oySetDefaultProfile_       (oyPROFILE_e       type,
                            const char*       file_name)
{
  int r = 0;

  DBG_PROG_START

  if( type == oyASSUMED_WEB &&
      !strstr( file_name,"sRGB" ) )
  {
    WARNc_S(_("wrong profile for static web colour space selected, need sRGB"))
    return 1;
  }
  r = oySetProfile_ (file_name, type, 0);
  DBG_PROG_ENDE
  return r;
}

int
oySetDefaultProfileBlock_  (oyPROFILE_e type,
                            const char* file_name, void* mem, size_t size)
{
  int r;

  DBG_PROG_START

  r = oySetProfile_Block (file_name, mem, size, type, 0);
  DBG_PROG_ENDE
  return r;
}

/*int*
oyGroupSetGet            (oyGROUP_e group, int * count )
{
  return 0;
}*/

char*
oyGetDefaultProfileName_   (oyPROFILE_e       type,
                            oyAlloc_f         allocate_func)
{
  char* name = 0;
  
  DBG_PROG_START

  DBG_PROG_V( type )

  /* a static_profile */
  if(type == oyASSUMED_WEB) {
    oyAllocHelper_m_( name, char, MAX_PATH, allocate_func, return NULL );
    oySprintf_(name, OY_WEB_RGB);
    DBG_PROG_S( name )
    return name;
  }

#ifdef __APPLE__
  name = oyGetDefaultProfileNameOSX_( type );
#else
  {
    const oyOption_t_ * t = oyOptionGet_((oyWIDGET_e)type);
    if( !t || !t->config_string )
    {
      WARNc2_S( "%s %d", _("Option not supported type:"), type)
      return NULL;
    } else
      name = oyGetKeyString_( t->config_string, allocate_func );
  }
#endif

  if(name && name[0])
  {
    DBG_PROG_S(name);
    /* cut off the path part of a file name */
    if (oyStrrchr_ (name, OY_SLASH_C))
    {
      char * f = NULL;

      oyAllocHelper_m_( f, char, oyStrlen_(name) + 1, oyAllocateFunc_, return 0);
      oySprintf_( f, "%s", name );
      oySprintf_( name, "%s", oyStrrchr_ (f, OY_SLASH_C) + 1 );
      oyFree_m_(f);
    }
  } else {
    const oyOption_t_ * t = oyOptionGet_((oyWIDGET_e)type);
    if(t && t->default_string)
    {
      name = 0;
      oyAllocHelper_m_( name, char, oyStrlen_( t->default_string ) + 1,
                        allocate_func, return NULL );
      oySprintf_( name, "%s", t->default_string );
    } else {
      WARNc2_S( "%s %d", _("Option not supported type:"), type)
    }
  }

  DBG_PROG_ENDE
  return name;
}


/* profile handling API */

int
oySetProfile_Block (const char* name, void* mem, size_t size,
                    oyPROFILE_e type, const char* comnt)
{
  int r = 0, len = 0;
  char *fullFileName = NULL, *resolvedFN = NULL;
  const char *fileName = NULL;

  DBG_PROG_START

  if (oyStrrchr_ (name, OY_SLASH_C))
    fileName = oyStrrchr_ (name, OY_SLASH_C);
  else
    fileName = name;

  oyAllocHelper_m_( fullFileName, char,
                    oyStrlen_(OY_PROFILE_PATH_USER_DEFAULT) +
                      oyStrlen_ (fileName) + 4,
                    oyAllocateFunc_, return 1);

  oySprintf_ (fullFileName, "%s%s", OY_PROFILE_PATH_USER_DEFAULT, OY_SLASH);
  len = oyStrlen_(fullFileName);
  memcpy( &fullFileName[len], fileName, oyStrlen_(fileName) );
  fullFileName[len + oyStrlen_(fileName)] = 0;

  resolvedFN = oyResolveDirFileName_ (fullFileName);
  oyFree_m_(fullFileName);
  fullFileName = resolvedFN;

  if (!oyCheckProfileMem_( mem, size, 0))
  {
    DBG_PROG_S(fullFileName)
    if ( oyIsFile_(fullFileName) ) {
      WARNc_PROFILE_S( fullFileName,
                _("file exists, please remove befor installing new profile."))
    } else
    { r = oyWriteMemToFile_ (fullFileName, mem, size);
      oySetProfile_ ( name, type, comnt);
    }
  }

  DBG_PROG1_S("%s", name)
  DBG_PROG1_S("%s", fileName)
  DBG_PROG2_S("%ld %d", (long int)&((char*)mem)[0] , (int)size)
  oyFree_m_(fullFileName);

  DBG_PROG_ENDE
  return r;
}

/* --- internal API decoupling --- */

#include "oyranos.h"

/** \addtogroup options Options API
 *  The idea behind this API is to provide one layout for
 *  presenting a configuration dialog to users. The advantage is, every 
 *  application, like KDE and Gnome control panels, will inherit the same logic.
 *  A user can easily use the one and the other panel
 *  without too much relearning.

 *  To illustrate a bit:
 *  In analogy to the WWW, I suggest to implement something like a minimalistic
 *  HTML display.
 *  Once the HTML alike content logic is done in Oyranos, the HTML page works
 *  and appears everywhere compareable. Smaller changes to the Oyranos HTML 
 *  alike pages will appear everywhere in all HTML page display programms.
 *  To remain in the WWW analogy, CSS would be optional to adapt visually.

 *  Functions are provided to set and query for Options layout and
 *  UI strings in Oyranos.

 *  @{
 */


/** @brief all widgets of a group
 *
 *  @param       group           specify which group dialog to build
 *  @param[out]  count           number of widgets contained in list
 *
 *  @return                      list of widgets to create in correct order
 */
oyWIDGET_e    * oyWidgetListGet          (oyGROUP_e           group,
                                        int             * count,
                                        oyAlloc_f         allocate_func )
{
  oyWIDGET_e *list = NULL;

  DBG_PROG_START
  oyExportStart_(EXPORT_CHECK_NO);
  oyTextsCheck_ ();

  list = oyWidgetListGet_                  ( group, count, allocate_func);

  oyExportEnd_();
  DBG_PROG_ENDE
  return list;
}

/** @brief Get a special Options UI strings.
 *
 *  @param       option          merges oyBEHAVIOUR_e and oyPROFILE_e
 *  @param[out]  categories      enums list, to place into the right group widget
 *                              { n, first category, second c., ... , n'th c. } 
 *                              for widget of type oyGROUP_e this is empty
 *  @param[out]  name            transated widget title
 *  @param[out]  tooltip         transated tooltip
 *
 *  @return                      widget type, gives a hint to further properties,
 *                              { like choices or int/float value ranges ... }
 */
oyWIDGET_TYPE_e oyWidgetTitleGet         (oyWIDGET_e          option,
                                        const oyGROUP_e  ** categories,
                                        const char     ** name,
                                        const char     ** tooltip,
                                        int             * flags )
{
  oyWIDGET_TYPE_e type = oyWIDGETTYPE_START;

  DBG_PROG_START
  oyExportStart_(EXPORT_CHECK_NO);
  oyTextsCheck_ ();

  type = oyWidgetTitleGet_                 ( option,
                                             categories,
                                             name, tooltip,
                                             flags );
  oyExportEnd_();
  DBG_PROG_ENDE
  return type;
}

oyWIDGET_TYPE_e  oyWidgetDescriptionGet (
                                       oyWIDGET_e          option,
                                       const char       ** description,
                                       int                 choice )
{
  oyWIDGET_TYPE_e type = oyWIDGETTYPE_START;

  DBG_PROG_START
  oyExportStart_(EXPORT_CHECK_NO);
  oyTextsCheck_ ();

  type = oyWidgetDescriptionGet_           ( option, description, choice );

  oyExportEnd_();
  DBG_PROG_ENDE
  return type;
}

/** @brief Get a Options choices UI strings.
 *
 *  This function is checked for double occuring profiles. Such are sorted out.
 *  @todo In the future a second choices_string_list may appear for displaying.
 *
 *  @param       option          merge oyBEHAVIOUR_e and oyPROFILE_e
 *  @param[out]  choices         n choices; if choices is zero then you need to
 *                              optain the choices otherwise, like for profiles
 *  @param[out]  choices_strings translated list of n choices
 *  @param[out]  current         the actual setting
 *
 *  @return                      success
 */
int         oyOptionChoicesGet         (oyWIDGET_e          option,
                                        int             * choices,
                                        const char    *** choices_string_list,
                                        int             * current )
{
  int error = 0;

  DBG_PROG_START
  oyExportStart_(EXPORT_PATH | EXPORT_SETTING);
  oyTextsCheck_ ();

  error =   oyOptionChoicesGet_            ( option,
                                             choices, choices_string_list,
                                             current );
  oyExportEnd_();
  DBG_PROG_ENDE
  return error;
}

/** @brief delete list of choices from a option
 *  @param list     string list
 *  @param size     number of strings in the list to free
 */
void
oyOptionChoicesFree                  (oyWIDGET_e        option,
                                      char          *** list,
                                      int               size)
{ DBG_PROG_START
  oyExportStart_(EXPORT_CHECK_NO);

  oyOptionChoicesFree_ (option, list, size);

  oyExportEnd_();
  DBG_PROG_ENDE
}

/*  @} */


/** \addtogroup behaviour Behaviour API
 *  Functions to set and query for behaviour on various actions in Oyranos.

 *  @todo is the missing description of available choices<br>

 *  Take a look at the Oyranos settings xml file. Options are described <a href="../../../color/settings/office.policy.xml">there</a>.
 
 *  Otherwise look at the \b oyranos-config-fltk application
    or the description on

 *  <a href="http://www.oyranos.com/wiki/index.php?title=Oyranos_Use_Cases">ColourWiki</a>. 

 *  @{
 */

/** Set a special behaviour. Usual in control panel in Oyranos.\n 
 *
 *  @param  type      the type of behaviour
 *  @param  choice    the selected option
 *  @return error
 */
int
oySetBehaviour         (oyBEHAVIOUR_e       type,
                        int               choice)
{
  int error = 0;

  DBG_PROG_START
  oyExportStart_(EXPORT_SETTING);
  oyTextsCheck_ ();

  error = oySetBehaviour_(type, choice);

  oyExportEnd_();
  DBG_PROG_ENDE
  return error;
}

/** Get a special behaviour.\n 

 *  @todo The options should silently fallback to defaults.

 *
 *  @param  type      the type of behaviour
 *  @return           the behaviour option (-1 if not available or not set)
 */
int
oyGetBehaviour         (oyBEHAVIOUR_e       type)
{
  int n = 0;

  DBG_PROG_START
  oyExportStart_(EXPORT_SETTING);
  oyTextsCheck_ ();

  n = oyGetBehaviour_(type);

  oyExportEnd_();
  DBG_PROG_ENDE
  return n;
}

/*  @} */

/** \addtogroup policy Policy API
 *  Functions to set and export policies in Oyranos.
 *
 *  Policies are a set of options used to group colour management settings.
 *
 *  The core API consists of oyReadXMLPolicy() and its reversal oyPolicyToXML().
 *
 *  Additional oyPolicySaveActual() is used to store the actual options into a
 *  policy, oyPolicyFileNameGet() to obtain the file name for a given policy
 *  name and oyPolicySet() to import the actual policy from a file.
 *
 *  For getting the count, names and actual one of the installed policy files
 *  use oyOptionChoicesGet() e.g.
 *  oyOptionChoicesGet( oyWIDGET_POLICY, &count, &names, &current )
 *
 *  @par Future:
 *       The policies are abstracted into the backend API oyCMMapi9_s. The 
 *       layout of these options can be obtained from the according backends.
 *       The colour management policy module for Oyranos is named "oicc".
 *
 *  @todo define some default policies internally
 *
 *  @{
 */

/** @brief save a options group to a file
 *
 *  The function is basically a wrapper for oyPolicyToXML() and will write
 *  the resulting XML to a file somewhere in XDG_CONFIG_HOME.
 *
 *  A convenience function.
 *
 *  @param         group               use oyGROUP_ALL for a typical snapshot
 *  @param         name                the name will become part of a filename
 *  @return                            error
 *
 *  @version Oyranos: 0.1.8
 *  @since   2008/07/23 (Oyranos: 0.1.8)
 *  @date    2008/07/23
 */
int         oyPolicySaveActual        ( oyGROUP_e         group,
                                        const char      * name )
{
  int error = !name;
  char * text = 0;
  char * filename = 0;
  const char * xdg_home_dir = getenv("XDG_CONFIG_HOME");
  char * path = 0, * ptr;

  DBG_PROG_START
  oyExportStart_(EXPORT_SETTING);
  oyTextsCheck_ ();

  if(!error)
  {
    text = oyPolicyToXML_(group, 1, oyAllocateFunc_);
    error = !text;
  }

  if(!error)
  {
    if(xdg_home_dir)
    {
      path = oyStringCopy_( xdg_home_dir, oyAllocateFunc_ );
      ptr = oyStrchr_( path, ':' );
      if(ptr)
        *ptr = '\000';
      oyStringAdd_( &filename, path, oyAllocateFunc_, oyDeAllocateFunc_ );
    } else
      oyStringAdd_( &filename, "~/.config", oyAllocateFunc_, oyDeAllocateFunc_ );

    oyStringAdd_( &filename, "/color/settings/", oyAllocateFunc_, oyDeAllocateFunc_ );
    oyStringAdd_( &filename, name, oyAllocateFunc_, oyDeAllocateFunc_ );
    oyStringAdd_( &filename, ".xml", oyAllocateFunc_, oyDeAllocateFunc_ );
    if(oyIsFile_(filename))
      WARNc2_S("%s %s",_("will overwrite policy file"), filename);
    error = oyWriteMemToFile_( filename, text, oyStrlen_(text)+1 );
  }

  oyExportEnd_();
  DBG_PROG_ENDE
  return error;
}

/** Save a group of policy settings.\n
 *  Write only such variables, which are available and ignore unknown ones.
 *  This currently produces pseudo xml configuration files.
 *
 *  @param  group     the policy group
 *  @param  add_header     add description
 *  @param allocate_func user provided function for allocating the strings memory
 *  @return           the configuration as XML to save to file
 */
char*
oyPolicyToXML          (oyGROUP_e           group,
                        int               add_header,
                        oyAlloc_f         allocate_func)
{
  char* text = 0;

  DBG_PROG_START
  oyExportStart_(EXPORT_SETTING);
  oyTextsCheck_ ();

  text = oyPolicyToXML_(group, add_header, allocate_func);

  oyExportEnd_();
  DBG_PROG_ENDE
  return text;
}

/** Load a group of policy settings.\n
 *  use xml-ish input produced by oyPolicyToXML()
 *
 *  @param  group     the policy group
 *  @param  xml       xml configuration string
 *  @return           errors
 */
int
oyReadXMLPolicy        (oyGROUP_e           group,
                        const char       *xml)
{
  int n = 0;

  DBG_PROG_START
  oyExportStart_(EXPORT_SETTING);
  oyTextsCheck_ ();

  n = oyReadXMLPolicy_(group, xml);

  oyExportEnd_();
  DBG_PROG_ENDE
  return n;
}

/** @func    oyPolicyFileNameGet
 *  @brief   resolve the file name of a policy
 *
 *  The resolving is done by weak string matching. So it is possible to use
 *  short hands, e.g. "Photo" for "Photographer". The first letter is ignored.
 *  A convenience function.
 *
 *  @param[in]     policy_name         the selected policy
 *  @param[out]    full_name           the full file name of policy_name
 *  @param[in]     allocateFunc        optional user allocator
 *  @return                            error
 *
 *  @version Oyranos: 0.1.10
 *  @since   2010/02/28 (Oyranos: 0.1.10)
 *  @date    2010/02/28
 */
int        oyPolicyFileNameGet       ( const char        * policy_name,
                                       char             ** full_name,
                                       oyAlloc_f           allocateFunc )
{
  int error = 0;

  DBG_PROG_START
  oyExportStart_(EXPORT_SETTING);
  oyTextsCheck_ ();

  error = oyPolicyFileNameGet_( policy_name, full_name,
                                allocateFunc ? allocateFunc : oyAllocateFunc_ );

  oyExportEnd_();
  DBG_PROG_ENDE
  return error;
}

/** Load a group of policy settings.\n
 *  use xml-ish file input produced by oyPolicyToXML()
 *
 *  A convenience function.
 *
 *  @param  policy_file  the policy file, will be locked up in standard paths
 *  @param  full_name    file name including path
 *  @return              errors
 */
int
oyPolicySet                (const char      * policy_file,
                            const char      * full_name )
{
  int n = 0;

  DBG_PROG_START
  oyExportStart_(EXPORT_SETTING);
  oyTextsCheck_ ();

  n = oyPolicySet_(policy_file, full_name);

  oyExportEnd_();
  DBG_PROG_ENDE
  return n;
}

/*  @} */

/** \addtogroup default_profiles Default Profiles API
 *  Functions to handle and configure default profiles.

 *  @see @ref path_names

 *  @{
 */

/** Sets a profile, which is available in the current configured path.
 *
 *  @param  type      the kind of default profile
 *  @param  file_name the profile which shall become the default for the above
 *                    specified profile type
 *  @return success
 */
int
oySetDefaultProfile        (oyPROFILE_e       type,
                            const char*       file_name)
{
  int n = 0;

  DBG_PROG_START
  oyExportStart_(EXPORT_PATH | EXPORT_SETTING);
  oyTextsCheck_ ();

  n = oySetDefaultProfile_ (type, file_name);

  oyExportEnd_();
  DBG_PROG_ENDE
  return n;
}

/** Sets a profile, which is available in the current configured path.
 *
 *  @param  type      the kind of default profile
 *  @param  file_name the profile which shall become the default for the above
 *          specified profile type
 *  @param  mem       the memory containing the profile
 *  @param  size      the profile size in mem
 *  @return success
 */
int
oySetDefaultProfileBlock   (oyPROFILE_e       type,
                            const char*       file_name,
                            void*             mem,
                            size_t            size)
{
  int n = 0;

  DBG_PROG_START
  oyExportStart_(EXPORT_PATH | EXPORT_SETTING);
  oyTextsCheck_ ();

  n = oySetDefaultProfileBlock_ (type, file_name, mem, size);

  oyExportEnd_();
  DBG_PROG_ENDE
  return n;
}

/** Gets a default profile filename.
 *
 *  @param  type          the kind of default profile
 *  @param  allocate_func the allocation function needed to reserve memory for
                          the filename string
 *  @return filename
 */
char*
oyGetDefaultProfileName    (oyPROFILE_e       type,
                            oyAlloc_f         allocate_func)
{
  char* name = NULL;

  DBG_PROG_START
  oyExportStart_(EXPORT_PATH | EXPORT_SETTING);
  oyTextsCheck_ ();

  name = oyGetDefaultProfileName_ (type, allocate_func);

  oyExportEnd_();
  DBG_PROG_ENDE
  return name;
}
/*  @} */


/** \addtogroup profile_lists Profile Lists API
 *  Functions to handle profile name lists.

 *  @see @ref path_names

 *  @{
 */

/** @brief get a list of profile filenames
 *  @param coloursig filter as ICC 4 byte string
 *  @param[out] size profile filenames count
 *  @return the profiles filename list allocated within Oyranos
 *
 *  @see @ref path_names
 */
char **  oyProfileListGet            ( const char        * coloursig,
                                       uint32_t          * size,
                                       oyAlloc_f           allocateFunc )
{
  char **names = NULL,
       ** tmp = 0;
  uint32_t n = 0;
  int tmp_n = 0;

  DBG_PROG_START
  oyExportStart_(EXPORT_PATH | EXPORT_SETTING);
  oyTextsCheck_ ();

  if(!allocateFunc)
    allocateFunc = oyAllocateFunc_;

  tmp = oyProfileListGet_(coloursig, &n);
  names = oyStringListAppend_( (const char**)tmp, n, 0,0, &tmp_n,
                               allocateFunc );

  oyStringListRelease_( &tmp, n, oyDeAllocateFunc_ );

  *size = tmp_n;

  oyExportEnd_();
  DBG_PROG_ENDE
  return names;
}

/** @} */


/** \addtogroup profile_handling Profile Handling API
 *  Functions to handle profiles.

 *  @{
 */

/** @brief check an file if it is a profile
 *
 *  @param  filename  the filename to check
 *  @param  coloursig colour space signature as described in the
                      <a href="http://www.color.org">ICC specification</a>
 *  @return true/false
 */
int
oyCheckProfile (const char* filename, const char* coloursig)
{
  int n = 0;

  DBG_PROG_START
  oyExportStart_(EXPORT_PATH | EXPORT_SETTING);
  oyTextsCheck_ ();

  /* coloursig is currently ignored */
  n = oyCheckProfile_ (filename, coloursig);

  oyExportEnd_();
  DBG_PROG_ENDE
  return n;
}

/** @brief check an memory block if it is a profile
 *
 *  @param  mem  the memory containing the profile
 *  @param  size the size of the memory block
 *  @param  coloursig currently ignored
 *  @return true/false
 */
int
oyCheckProfileMem (const void* mem, size_t size, const char* coloursig)
{
  int n = 0;

  DBG_PROG_START
  oyExportStart_(EXPORT_CHECK_NO);
  oyTextsCheck_ ();

  /* coloursig is currently ignored */
  n = oyCheckProfileMem_ (mem, size, coloursig);

  oyExportEnd_();
  DBG_PROG_ENDE
  return n;
}

/** @brief get the profiles size
 *
 *  @param  profilename  specifies the profile
 *  @return size
 */
size_t
oyGetProfileSize                  (const char* profilename)
{
  size_t size = 0;

  DBG_PROG_START
  oyExportStart_(EXPORT_PATH | EXPORT_SETTING);

  size = oyGetProfileSize_ (profilename);

  oyExportEnd_();
  DBG_PROG_ENDE
  return size;
}

/** @brief obtain an memory block in the responsibility of the user
 *
 *  Please note the size parameter is sensible. Eighter set it to zero or to  
 *  the size obtained with oyGetProfileSize to get the full data or
 *  specify the size you want to get a partitial profile. \n
 *  This feature is useful for instance to get only the headers of many profiles.
 *
 *  @param  profilename  specifies the profile
 *  @param[in]  size     desired size, or zero for a complete copy
    @param[out] size     the size of the returned block
 *  @param      allocate_func the users memory allocation function
 *  @return             the profile block in memory allocated by oyAlloc_f    
 */
void*
oyGetProfileBlock                 (const char* profilename, size_t *size,
                                   oyAlloc_f     allocate_func)
{
  char* block = NULL;

  DBG_PROG_START
  oyExportStart_(EXPORT_PATH | EXPORT_SETTING);

  block = oyGetProfileBlock_ (profilename, size, allocate_func);
  DBG_PROG3_S( "%s %hd %d", profilename, (int)(intptr_t)block, (int)(intptr_t)*size)
  DBG_PROG

  oyExportEnd_();
  DBG_PROG_ENDE
  return block;
}

#if 1
/** Find out where in the Oyranos search path the specified profile resides.
 *
 *  @deprecated This function will be substituded by oyProfile_GetFileName.
 *
 *  @param  profile_name  the filename find in the Oyranos search path
 *  @param  allocate_func user provided function for allocating the string
                          memory
 *  @return the path name where the profile was found in the Oyranos search path
 */
char*
oyGetPathFromProfileName (const char* profile_name, oyAlloc_f     allocate_func)
{
  char* path_name = NULL;

  DBG_PROG_START
  oyExportStart_(EXPORT_PATH | EXPORT_SETTING);

  path_name = oyGetPathFromProfileName_ (profile_name, allocate_func);

  oyExportEnd_();
  DBG_PROG_ENDE
  return path_name;
}
#endif

/** @brief  calculate a md5 digest beginning after the header offset
 *
 *  The md5 implementation is written by L. Peter Deutsch
 *
 *  @param[in]  buffer         complete profiles buffer
 *  @param[in]  size           over all profile size
 *  @param[out] md5_return     buffer to write in the md5 digest (128 bytes)
 *  @return                    -1 wrong profile_id detected, 0 - good, 1 - error
 *
 *  @return                    error
 */
int
oyProfileGetMD5        ( void       *buffer,
                         size_t      size,
                         unsigned char *md5_return )
{
  int error = 0;
  DBG_PROG_START
  oyExportStart_(EXPORT_CHECK_NO);

  error = oyProfileGetMD5_(buffer, size, md5_return);

  oyExportEnd_();
  DBG_PROG_ENDE
  return error;
}
/** @} */

/**
 *  @internal
 *  \addtogroup cmm_handling CMM Handling API
 *
 *  @{
 */

/** @} */

/** \addtogroup misc Miscellaneous
 *  Miscellaneous stuff.

 *  @{
 */

/** @brief  switch internationalisation of strings on or off
 *
 *  @param  active         bool
 *  @param  reserved       for future use
 */
void 
oyI18NSet              ( int active,
                         int reserved )
{
  DBG_PROG_START
  oyInit_();

  oyI18NSet_(active, reserved);

  oyExportEnd_();
  DBG_PROG_ENDE
}


/** @} */



