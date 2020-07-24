#pragma once

#if defined(_MSC_VER)
#   include <winapifamily.h>
#endif

/*
 * Debug detection
 */

#if (!defined(DEBUG) && !defined(_DEBUG)) || defined(NDEBUG)
#	define CGULL_NO_DEBUG
#endif

/*
 * OS detection
 */

#if defined(__ANDROID__) || defined(ANDROID)
#   define CGULL_OS_ANDROID
#   define CGULL_OS_LINUX
#elif defined(__CYGWIN__)
#   define CGULL_OS_CYGWIN
#elif !defined(SAG_COM) && (!defined(WINAPI_FAMILY) || WINAPI_FAMILY==WINAPI_FAMILY_DESKTOP_APP) && (defined(WIN64) || defined(_WIN64) || defined(__WIN64__))
#   define CGULL_OS_WIN32
#   define CGULL_OS_WIN64
#elif !defined(SAG_COM) && (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__))
#   if defined(WINAPI_FAMILY)
#       if !defined(WINAPI_FAMILY_PC_APP)
#           define WINAPI_FAMILY_PC_APP WINAPI_FAMILY_APP
#       endif
#       if defined(WINAPI_FAMILY_PHONE_APP) && WINAPI_FAMILY==WINAPI_FAMILY_PHONE_APP
#           define CGULL_OS_WINRT
#       elif WINAPI_FAMILY==WINAPI_FAMILY_PC_APP
#           define CGULL_OS_WINRT
#       else
#           define CGULL_OS_WIN32
#       endif
#   else
#       define CGULL_OS_WIN32
#   endif
#elif defined(__linux__) || defined(__linux)
#   define CGULL_OS_LINUX
#elif defined(__FreeBSD__) || defined(__DragonFly__) || defined(__FreeBSD_kernel__)
#elif defined(__NetBSD__)
#elif defined(__OpenBSD__)
#elif defined(__INTERIX)
#elif defined(_AIX)
#elif defined(__Lynx__)
#elif defined(__GNU__)
#elif defined(__QNXNTO__)
#elif defined(__INTEGRITY)
#elif defined(__rtems__)
#elif defined(__HAIKU__)
#elif defined(__MAKEDEPEND__)
#else
#   error "Unknown OS"
#endif

#if defined(CGULL_OS_WIN32) || defined(CGULL_OS_WIN64) || defined(CGULL_OS_WINRT)
#   define CGULL_OS_WIN
#endif

#if defined(CGULL_OS_WIN)
#   undef CGULL_OS_UNIX
#elif !defined(CGULL_OS_UNIX)
#   define CGULL_OS_UNIX
#endif

/*
 * Pointer size detection
 */
#if defined __SIZEOF_POINTER__
#  define CGULL_POINTER_SIZE            __SIZEOF_POINTER__
#elif defined(__LP64__) || defined(_LP64)
#  define CGULL_POINTER_SIZE            8
#elif defined(__arm__) || defined(__TARGET_ARCH_ARM) || defined(_M_ARM) || defined(_M_ARM64) || defined(__aarch64__) || defined(__ARM64__)
#   if defined(__aarch64__) || defined(__ARM64__) || defined(_M_ARM64)
#       define CGULL_POINTER_SIZE       8
#   endif
#elif defined(__i386) || defined(__i386__) || defined(_M_IX86)
#   define CGULL_POINTER_SIZE           4
#elif defined(__x86_64) || defined(__x86_64__) || defined(__amd64) || defined(_M_X64)
#   define CGULL_POINTER_SIZE           8
#elif defined(__ia64) || defined(__ia64__) || defined(_M_IA64)
#   define CGULL_POINTER_SIZE           8
#elif defined(__mips) || defined(__mips__) || defined(_M_MRX000)
#   if defined(_MIPS_ARCH_MIPS64) || defined(__mips64)
#       define CGULL_POINTER_SIZE       8
#   endif
#elif defined(__ppc__) || defined(__ppc) || defined(__powerpc__) || defined(_ARCH_COM) || defined(_ARCH_PWR) || defined(_ARCH_PPC) || defined(_M_MPPC) || defined(_M_PPC)
#   if defined(__ppc64__) || defined(__powerpc64__) || defined(__64BIT__)
#       define CGULL_POINTER_SIZE       8
#   endif
#elif defined(__EMSCRIPTEN__)
#   define CGULL_POINTER_SIZE           8
#else
#   define CGULL_POINTER_SIZE           4
#endif
