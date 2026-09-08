# ---- Developer mode ----

# Developer mode enables targets and code paths in the CMake scripts that are
# only relevant for the developer(s) of speedgun-ng
# Targets necessary to build the project must be provided unconditionally, so
# consumers can trivially build and package the project
if(PROJECT_IS_TOP_LEVEL)
  option(speedgun-ng_DEVELOPER_MODE "Enable developer mode" OFF)
  option(BUILD_SHARED_LIBS "Build shared libs." OFF)
endif()

# ---- Suppress C4251 on Windows ----

# Please see include/speedgun-ng/speedgun-ng.hpp for more details
set(pragma_suppress_c4251 "
/* This needs to suppress only for MSVC */
#if defined(_MSC_VER) && !defined(__ICL)
#  define SPEEDGUN_NG_SUPPRESS_C4251 _Pragma(\"warning(suppress:4251)\")
#else
#  define SPEEDGUN_NG_SUPPRESS_C4251
#endif
")

# ---- Warnings as errors (constitution VIII) ----
# Top-level builds never treat diagnostics as advisory. Consumers who
# add_subdirectory this project do not inherit the error promotion.
if(PROJECT_IS_TOP_LEVEL)
  set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
  if(CMAKE_VERSION VERSION_LESS "3.24")
    if(MSVC)
      add_compile_options(/WX)
    else()
      add_compile_options(-Werror)
    endif()
  endif()
endif()

# ---- Warning guard ----

# target_include_directories with the SYSTEM modifier will request the compiler
# to omit warnings from the provided paths, if the compiler supports that
# This is to provide a user experience similar to find_package when
# add_subdirectory or FetchContent is used to consume this project
set(warning_guard "")
if(NOT PROJECT_IS_TOP_LEVEL)
  option(
      speedgun-ng_INCLUDES_WITH_SYSTEM
      "Use SYSTEM modifier for speedgun-ng's includes, disabling warnings"
      ON
  )
  mark_as_advanced(speedgun-ng_INCLUDES_WITH_SYSTEM)
  if(speedgun-ng_INCLUDES_WITH_SYSTEM)
    set(warning_guard SYSTEM)
  endif()
endif()
