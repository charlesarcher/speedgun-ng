# ---- simdjson static archive merge ----
#
# Contract: specs/004-vendor-simdjson/contracts/build-integration.md
# Research: specs/004-vendor-simdjson/research.md (R-009)
#
# The merge mechanism lives in cmake/VendoredArchiveMerge.cmake
# (specs/005-vendor-hdrhistogram R-006); this module is the named
# wrapper for the simdjson call site. The contract is unchanged: the
# post-build step merges the vendored libsimdjson.a into the static
# library so the installed archive stays self-contained, the gate's
# get_active_implementation reference resolves in-archive (SC-009),
# and the package files name nothing foreign (FR-015).
#
# Usage:
#
#   include(cmake/SimdjsonArchiveMerge.cmake)
#   simdjson_archive_merge(<static-library-target>)

include("${CMAKE_CURRENT_LIST_DIR}/VendoredArchiveMerge.cmake")

function(simdjson_archive_merge merge_into)
  vendored_archive_merge(${merge_into} simdjson)
endfunction()
