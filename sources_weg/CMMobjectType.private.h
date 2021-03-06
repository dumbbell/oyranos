/** typedef oyCMMobjectLoadFromMem_f
 *  @brief   load a filter object from a in memory data blob
 *  @ingroup module_api
 *  @memberof oyCMMobjectType_s
 *
 *  @param[in]     buf_size            data size
 *  @param[in]     buf                 data blob
 *  @param[in]     flags               for future use
 *  @param[in]     object              the optional base
 *  @return                            the object
 *
 *  @version Oyranos: 0.1.10
 *  @since   2008/11/22 (Oyranos: 0.1.9)
 *  @date    2009/09/14
 */
typedef oyStruct_s * (*oyCMMobjectLoadFromMem_f) (
                                       size_t              buf_size,
                                       const oyPointer     buf,
                                       uint32_t            flags,
                                       oyObject_s          object);

/** typedef oyCMMobjectGetText_f
 *  @brief   build a text string from a given object
 *  @ingroup module_api
 *  @memberof oyCMMobjectType_s
 *
 *  Serialise a object into:
 *  - oyNAME_NICK: XML ID
 *  - oyNAME_NAME: XML
 *  - oyNAME_DESCRIPTION: ??
 *
 *  For type information the object argument is omitted. Then the function shall
 *  programatically tell in oyNAME_NICK about the object type,
 *  e.g. "oyProfile_s",
 *  in oyNAME_NAME translated about its intented usage,
 *  e.g. i18n("ICC profile") and give with oyNAME_DESCRIPTION some further long
 *  informations.
 *
 *  Note: Dynamically generated informations can be stored in the
 *  oyStruct_s::oy::name_  member and then returned by the function. Oyranos
 *  will typical look first at that oyObject_s member and then ask this
 *  function to get the information. @see oyObject_SetName()
 *
 *  @param[in]     object              the object, omit to get a general text
 *  @param[out]    type                the string type
 *  @param[in]     flags               for future use
 *  @return                            0 on success; error >= 1; unknown < 0
 *
 *  @version Oyranos: 0.1.10
 *  @since   2008/12/24 (Oyranos: 0.1.10)
 *  @date    2009/09/14
 */
typedef const char *   (*oyCMMobjectGetText_f) (
                                       oyStruct_s        * object,
                                       oyNAME_e            type,
                                       int                 flags );

/** typedef oyCMMobjectScan_f
 *  @brief   load a filter object from a in memory data blob
 *  @ingroup module_api
 *  @memberof oyCMMobjectType_s
 *
 *  @param[in]     data                data blob
 *  @param[in]     size                data size
 *  @param[out]    registration        filter registration string
 *  @param[out]    name                filter name
 *  @param[in]     allocateFunc        e.g. malloc
 *  @return                            0 on success; error >= 1; unknown < 0
 *
 *  @version Oyranos: 0.1.10
 *  @since   2008/11/22 (Oyranos: 0.1.9)
 *  @date    2009/09/14
 */
typedef int          (*oyCMMobjectScan_f) (
                                       oyPointer           data,
                                       size_t              size,
                                       char             ** registration,
                                       char             ** name,
                                       oyAlloc_f           allocateFunc );
