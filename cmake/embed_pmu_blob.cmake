# Compiles the vendored event tables into static data (specs/012, US5).
# Run as `cmake -P` with EMBED_ROOT and EMBED_OUT. Emits one translation
# unit per vendored directory plus one registry, and never installs
# anything: the library owns its tables from the moment it links
# (FR-036). Every byte reaches the generated literal unchanged, so the
# rows the parser sees are the rows the vendored tree holds.

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
    embed_escape(file_escaped "${file_hex}")
    string(APPEND index "  {\"${file_name}\", ${offset}UL, ${file_bytes}UL},\n")
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
    " kEmbFileCount${camel}},\n")
  string(CONCAT decl
    "extern const char kEmbData${camel}[];\n"
    "extern const std::size_t kEmbSize${camel};\n"
    "extern const EmbeddedFile kEmbFiles${camel}[];\n"
    "extern const std::size_t kEmbFileCount${camel};\n")
  set(entry "${entry}" PARENT_SCOPE)
  set(decl "${decl}" PARENT_SCOPE)
endfunction()

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
"${decls}"
"extern const EmbeddedDir kEmbeddedDirs[] = {\n${registry}};\n"
"extern const std::size_t kEmbeddedDirCount =\n"
"  sizeof kEmbeddedDirs / sizeof kEmbeddedDirs[0];\n"
"}  // namespace sg::counters::detail\n")

message(STATUS "embedded ${arch_name} directories: ${symbols}")
