#pragma once

// Embedded vendor event tables (specs/012-counters-defect-resolution,
// US5, FR-036). The vendored JSON under external/pmu-events is compiled
// into the library as static data at build time, so an installed
// package carries its tables and the library performs no run-time path
// lookup for them. The seam is private: never installed, and the sole
// holder of the generated symbols.
//
// One directory of the vendored tree becomes one generated translation
// unit holding that directory's bytes and an index over them. The
// generated registry lists every directory beside them.

#include <cstddef>
#include <cstring>
#include <string_view>

namespace sg::counters::detail
{

// One vendored JSON file, named by its path relative to the directory
// that holds it. The bytes live in the directory's own buffer.
struct EmbeddedFile
{
  const char* name;
  std::size_t offset;
  std::size_t length;
};

// One directory of the vendored tree, named by its path relative to the
// vendored root. The mapfile sits under arch/x86 alone, so it reaches
// the caller as that directory's single file.
struct EmbeddedDir
{
  const char* path;
  const char* data;
  std::size_t size;
  const EmbeddedFile* files;
  std::size_t fileCount;
};

// Defined by the generated registry, one entry per vendored directory.
extern const EmbeddedDir kEmbeddedDirs[];
extern const std::size_t kEmbeddedDirCount;

// Finds the embedded directory a vendored path names, or nullptr when
// the tree holds no such directory. Inline, so the lookup needs no
// translation unit of its own.
inline auto embeddedFindDir(std::string_view path) noexcept
    -> const EmbeddedDir*
{
  for (std::size_t i = 0; i < kEmbeddedDirCount; ++i) {
    if (path == kEmbeddedDirs[i].path) {
      return &kEmbeddedDirs[i];
    }
  }
  return nullptr;
}

// The bytes of one file within an embedded directory, or an empty view
// when the directory holds no such file.
inline auto embeddedFileBytes(
    const EmbeddedDir& dir, std::string_view name) noexcept -> std::string_view
{
  for (std::size_t i = 0; i < dir.fileCount; ++i) {
    if (name == dir.files[i].name) {
      return std::string_view {dir.data + dir.files[i].offset,
                               dir.files[i].length};
    }
  }
  return {};
}

}  // namespace sg::counters::detail
