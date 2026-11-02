# Compiles the vendored event tables into static data (specs/012, US5).
# Run as `cmake -P` with EMBED_ROOT, EMBED_OUT and EMBED_RECORD. Emits one
# translation unit per vendored directory plus one registry, and never
# installs anything: the library owns its tables from the moment it links
# (FR-036). Every byte reaches the generated literal unchanged, so the
# rows the parser sees are the rows the vendored tree holds.
#
# EMBED_RECORD names the manifest the vendored tree ships. Its `ref:` line
# reaches the registry as the reference of every directory, and its digest
# line reaches the index entry of the file it names, so the library carries
# the source of its own tables and no digest is retyped by hand (specs/042
# FR-015, spec D-12, PC-12). A file the manifest names no digest for fails
# the build: a table a manifest does not vouch for is data no reader can
# trace, and a silent gap there is the defect PC-12 records.

cmake_minimum_required(VERSION 3.20)

# Escapes bytes as hexadecimal C++ escapes. Every escape opens with a
# backslash and no two hex digits ever abut, so no escape can swallow the
# one after it.
function(embed_escape out_hex in_hex)
  string(REGEX REPLACE "(..)" "\\\\x\\1" escaped "${in_hex}")
  set(${out_hex} "${escaped}" PARENT_SCOPE)
endfunction()

# Camel-cases an underscore-separated symbol. CMake has no case escape in
# a regex replacement, so each part is uppercased through its first
# character instead. The symbol carries an architecture directory name
# into a C++ identifier, which the naming rule spells in CamelCase.
function(camel_symbol out in)
  string(REPLACE "_" ";" parts "${in}")
  set(acc "")
  foreach(part IN LISTS parts)
    string(SUBSTRING "${part}" 0 1 head)
    string(SUBSTRING "${part}" 1 -1 tail)
    string(TOUPPER "${head}" head)
    string(APPEND acc "${head}${tail}")
  endforeach()
  set(${out} "${acc}" PARENT_SCOPE)
endfunction()

# Reads the manifest once: the reference it states and one digest per
# vendored path. The path is what a lookup keys on, because the manifest
# spells a file by the path the vendored root gives it while the index names
# it by the bare name the directory gives it.
function(embed_read_manifest record out_ref out_paths out_hashes)
  file(STRINGS "${record}" lines ENCODING UTF-8)
  set(ref "")
  set(paths "")
  set(hashes "")
  foreach(line IN LISTS lines)
    if(line MATCHES "^ref: +([^ \n]+)")
      set(ref "${CMAKE_MATCH_1}")
    elseif(line MATCHES "^([0-9a-f]+)  (.+)$")
      list(APPEND paths "${CMAKE_MATCH_2}")
      list(APPEND hashes "${CMAKE_MATCH_1}")
    endif()
  endforeach()
  if(ref STREQUAL "")
    message(FATAL_ERROR "the manifest ${record} states no 'ref:' line")
  endif()
  list(LENGTH paths recorded)
  if(recorded EQUAL 0)
    message(FATAL_ERROR "the manifest ${record} states no digest line")
  endif()
  set(${out_ref} "${ref}" PARENT_SCOPE)
  set(${out_paths} "${paths}" PARENT_SCOPE)
  set(${out_hashes} "${hashes}" PARENT_SCOPE)
endfunction()

# The digest one vendored path stands under, and nothing when the manifest
# holds no line for it.
function(embed_manifest_digest rel_path out)
  list(FIND EMBED_PATHS "${rel_path}" found)
  if(found LESS 0)
    set(${out} "" PARENT_SCOPE)
    return()
  endif()
  list(GET EMBED_HASHES ${found} digest)
  set(${out} "${digest}" PARENT_SCOPE)
endfunction()

# Turns the files of one vendored directory into a translation unit. The
# files concatenate into one literal and an index over them records where
# each begins, so the bytes stay contiguous and a file is reachable
# without walking the file system. The pattern selects the files, which
# is how the mapfile embeds as arch/x86's single file. The symbol is the
# file stem; its CamelCase spelling names the emitted identifiers.
function(embed_directory dir_path rel_path symbol pattern)
  file(GLOB files "${dir_path}/${pattern}")
  list(SORT files)
  set(hex_all "")
  set(index "")
  set(offset 0)
  set(count 0)
  foreach(embed_file IN LISTS files)
    file(READ "${embed_file}" file_hex HEX)
    string(LENGTH "${file_hex}" file_hex_len)
    # A hex read spends two characters per byte, while the index counts
    # bytes, so the offset advances by half the hex length.
    math(EXPR file_bytes "${file_hex_len} / 2")
    get_filename_component(file_name "${embed_file}" NAME)
    file(RELATIVE_PATH manifest_path "${EMBED_ROOT}" "${embed_file}")
    embed_manifest_digest("${manifest_path}" file_digest)
    if(file_digest STREQUAL "")
      message(
          FATAL_ERROR
          "the manifest ${EMBED_RECORD} states no digest line for "
          "${manifest_path}: re-pin with tools/pmu_events/update_pmu_events.py "
          "or record the file, because a table nobody can trace is worse than "
          "no table at all"
      )
    endif()
    embed_escape(file_escaped "${file_hex}")
    string(APPEND index
      "  {\"${file_name}\", ${offset}UL, ${file_bytes}UL, \"${file_digest}\"},\n")
    string(APPEND hex_all "${file_escaped}")
    math(EXPR offset "${offset} + ${file_bytes}")
    math(EXPR count "${count} + 1")
  endforeach()

  if(count EQUAL 0)
    message(FATAL_ERROR "no ${pattern} under ${dir_path}")
  endif()

  camel_symbol(camel "${symbol}")
  file(WRITE "${EMBED_OUT}/${symbol}.cpp"
"#include \"counters/linux_pmu/embedded_tables.hpp\"\n\n"
"// Generated from ${rel_path}. Do not edit.\n"
"namespace sg::counters::detail\n{\n"
"extern const char kEmbData${camel}[] =\n\"${hex_all}\";\n"
"extern const std::size_t kEmbSize${camel} =\n  sizeof kEmbData${camel};\n"
"extern const EmbeddedFile kEmbFiles${camel}[] = {\n${index}};\n"
"extern const std::size_t kEmbFileCount${camel} =\n"
"  sizeof kEmbFiles${camel} / sizeof kEmbFiles${camel}[0];\n"
"}  // namespace sg::counters::detail\n")

  # One argument each: set() joins several with a semicolon, and the
  # semicolon would land inside the emitted source.
  string(CONCAT entry
    "  {\"${rel_path}\", kEmbData${camel},"
    " kEmbSize${camel},"
    " kEmbFiles${camel},"
    " kEmbFileCount${camel},"
    " kEmbeddedReference},\n")
  string(CONCAT decl
    "extern const char kEmbData${camel}[];\n"
    "extern const std::size_t kEmbSize${camel};\n"
    "extern const EmbeddedFile kEmbFiles${camel}[];\n"
    "extern const std::size_t kEmbFileCount${camel};\n")
  set(entry "${entry}" PARENT_SCOPE)
  set(decl "${decl}" PARENT_SCOPE)
endfunction()

if(NOT EMBED_RECORD)
  message(FATAL_ERROR "EMBED_RECORD must name the manifest of ${EMBED_ROOT}")
endif()
if(NOT EXISTS "${EMBED_RECORD}")
  message(FATAL_ERROR "no manifest at ${EMBED_RECORD} beside ${EMBED_ROOT}")
endif()
embed_read_manifest("${EMBED_RECORD}" EMBED_REF EMBED_PATHS EMBED_HASHES)

file(GLOB arch_dirs "${EMBED_ROOT}/arch/x86/*")
list(SORT arch_dirs)

set(registry "")
set(decls "")
set(symbols "")

# The mapfile is what names the directory for a CPU, so it embeds beside
# the directories and reaches the caller as arch/x86's single file.
embed_directory("${EMBED_ROOT}/arch/x86" "arch/x86" "x86_mapfile" "mapfile.csv")
string(APPEND registry "${entry}")
string(APPEND decls "${decl}")

foreach(arch_dir IN LISTS arch_dirs)
  if(NOT IS_DIRECTORY "${arch_dir}")
    continue()
  endif()
  get_filename_component(arch_name "${arch_dir}" NAME)
  # Two vendored directories carry a hyphen and a symbol cannot, so the
  # symbol folds it to an underscore while the path keeps its own name.
  string(REGEX REPLACE "[^A-Za-z0-9]" "_" arch_symbol "${arch_name}")
  embed_directory("${arch_dir}" "arch/x86/${arch_name}" "${arch_symbol}" "*.json")
  string(APPEND registry "${entry}")
  string(APPEND decls "${decl}")
  string(APPEND symbols "${arch_name} ")
endforeach()

file(WRITE "${EMBED_OUT}/registry.cpp"
"#include \"counters/linux_pmu/embedded_tables.hpp\"\n\n"
"// Generated from the vendored tree. Do not edit.\n"
"namespace sg::counters::detail\n{\n"
"// The reference the manifest states, one value for every directory of the\n"
"// tree it lists.\n"
"extern const char kEmbeddedReference[] = \"${EMBED_REF}\";\n"
"${decls}"
"extern const EmbeddedDir kEmbeddedDirs[] = {\n${registry}};\n"
"extern const std::size_t kEmbeddedDirCount =\n"
"  sizeof kEmbeddedDirs / sizeof kEmbeddedDirs[0];\n"
"}  // namespace sg::counters::detail\n")

message(STATUS "embedded ${arch_name} directories: ${symbols}")
