/*
 * EDG target configuration for MinGW-w64 GCC on x86_64 Windows, used when ROSE's backend compiler
 * is a MinGW-w64 GCC (the Makefile then defines EDG2SAGE_TARGET_MINGW; see edg_config.h).
 *
 * The default target of cmake_defines.h is x86_64 Linux.  MinGW-w64 GCC keeps the Itanium C++ ABI
 * and the 80-bit x87 long double of that target, but uses the Windows LLP64 data model (32-bit
 * long, 16-bit wchar_t, 64-bit size_t and ptrdiff_t) and, by default (-mms-bitfields), the
 * Microsoft layout of bit-fields.  (EDG's own "win64" target configuration describes Microsoft
 * Visual C++, whose long double is a 64-bit double.)
 */
#ifndef EDG2SAGE_MINGW_X86_64_H
#define EDG2SAGE_MINGW_X86_64_H

#define LEGACY_TARGET_CONFIGURATION_NAME "mingw_x86_64"

/* LLP64 */
#define TARG_SIZEOF_LONG 4
#define TARG_ALIGNOF_LONG 4
#define TARG_LONG_FIELD_ALIGNMENT 4
#define TARG_SIZE_T_INT_KIND ((an_integer_kind)ik_unsigned_long_long)
#define TARG_PTRDIFF_T_INT_KIND ((an_integer_kind)ik_long_long)
#define TARG_SSIZE_T_INT_KIND ((an_integer_kind)ik_long_long)
#define TARG_WCHAR_T_INT_KIND ((an_integer_kind)ik_unsigned_short)
#define TARG_WINT_T_INT_KIND ((an_integer_kind)ik_unsigned_short)

/* Values that EDG derives from the size of long by default */
#define TARG_SIZEOF_PTR_TO_DATA_MEMBER 8
#define TARG_ALIGNOF_PTR_TO_DATA_MEMBER 8
#define TARG_SIZEOF_PTR_TO_MEMBER_FUNCTION 16
#define TARG_ALIGNOF_PTR_TO_MEMBER_FUNCTION 8
#define TARG_SIZEOF_PTR_TO_VIRTUAL_BASE_CLASS 8
#define TARG_ALIGNOF_PTR_TO_VIRTUAL_BASE_CLASS 8
#define TARG_SIZEOF_LARGEST_ATOMIC 16
#define TARG_IA64_VTABLE_ENTRY_INT_KIND ((an_integer_kind)ik_long_long)
#define TARG_JMP_BUF_ELEMENT_INT_KIND ((an_integer_kind)ik_long_long)

/* GCC's word mode (__attribute__((mode(word))), etc.) is 64 bits on every x86_64 target */
#define TARG_WORD_MODE ((a_type_mode_kind)tmk_DI)
#define TARG_UNWIND_WORD_MODE ((a_type_mode_kind)tmk_DI)
#define TARG_LIBGCC_CMP_RETURN_MODE ((a_type_mode_kind)tmk_DI)
#define TARG_LIBGCC_SHIFT_COUNT_MODE ((a_type_mode_kind)tmk_DI)

/* Microsoft bit-field layout (as in EDG's Windows x86_64 configuration, except that, as with GCC,
   bit-fields also affect the alignment of unions).  __attribute__((gcc_struct)) is not emulated. */
#define TARG_MICROSOFT_BIT_FIELD_ALLOCATION 1
#define TARG_BIT_FIELD_AFFECTS_UNION_ALIGNMENT 1
#define TARG_UNNAMED_BIT_FIELD_AFFECTS_STRUCT_ALIGNMENT 1
#define TARG_ZERO_WIDTH_BIT_FIELD_AFFECTS_STRUCT_ALIGNMENT 1
#define TARG_ZERO_WIDTH_BIT_FIELD_ALIGNMENT (-1)

#endif
