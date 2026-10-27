#ifndef SG_HARNESS_PROC_ROOT_HPP
#define SG_HARNESS_PROC_ROOT_HPP

#include <filesystem>
#include <optional>
#include <string_view>
#include <utility>

namespace sg::detail
{

namespace proc_root_detail
{

/// The one storage the two seam functions share; no lock stands over
/// it, the standing shape of the install seams (R-02): one install
/// stands before the reads it feeds and no read runs while an install
/// changes.
inline auto storage() -> std::optional<std::filesystem::path>&
{
  static std::optional<std::filesystem::path> stored;
  return stored;
}

}  // namespace proc_root_detail

/// @brief The file root every `/proc` and `/sys` read of the pre-check
/// resolves under (specs/034 FR-019, R-02): the reads name one path
/// each, the path joined under the root, so the production reads name
/// the files the kernel owns and the tests read the files the seam
/// installs.
///
/// \pre `relative` names no absolute path
/// \post the returned path joins the active root and `relative`
[[nodiscard]] inline auto procRootPath(std::string_view relative)
    -> std::filesystem::path
{
  const auto& root = proc_root_detail::storage();
  const auto& base = root.has_value() ? *root
                                      : std::filesystem::path("/");
  return base / std::filesystem::path(relative);
}

/// @brief The test seam of the file root: an installed path stands in
/// for `/` so the tests read fake files, and an empty optional clears
/// the install, the way `installTestLayout` clears with an empty
/// optional (R-02, FR-019). The N13d and N13e lanes reuse this seam.
///
/// \pre the install stands before the first read it feeds, and no
///      read runs while the install changes (E-02)
/// \post `procRootPath` resolves under `root` while the install
///       stands, and under `/` after the clear
inline auto installTestProcRoot(std::optional<std::filesystem::path> root)
    -> void
{
  proc_root_detail::storage() = std::move(root);
}

}  // namespace sg::detail

#endif
