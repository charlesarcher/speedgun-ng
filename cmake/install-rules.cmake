if(PROJECT_IS_TOP_LEVEL)
  set(
      CMAKE_INSTALL_INCLUDEDIR "include/speedgun-ng-${PROJECT_VERSION}"
      CACHE STRING ""
  )
  set_property(CACHE CMAKE_INSTALL_INCLUDEDIR PROPERTY TYPE PATH)
endif()

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# find_package(<package>) call for consumers to find this project
set(package speedgun-ng)

install(
    DIRECTORY
    include/
    "${PROJECT_BINARY_DIR}/export/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT speedgun-ng_Development
)

install(
    TARGETS speedgun-ng_speedgun-ng
    EXPORT speedgun-ngTargets
    RUNTIME #
    COMPONENT speedgun-ng_Runtime
    LIBRARY #
    COMPONENT speedgun-ng_Runtime
    NAMELINK_COMPONENT speedgun-ng_Development
    ARCHIVE #
    COMPONENT speedgun-ng_Development
    INCLUDES #
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

write_basic_package_version_file(
    "${package}ConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion
)

# Allow package maintainers to freely override the path for the configs
set(
    speedgun-ng_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/${package}"
    CACHE STRING "CMake package config location relative to the install prefix"
)
set_property(CACHE speedgun-ng_INSTALL_CMAKEDIR PROPERTY TYPE PATH)
mark_as_advanced(speedgun-ng_INSTALL_CMAKEDIR)

install(
    FILES cmake/install-config.cmake
    DESTINATION "${speedgun-ng_INSTALL_CMAKEDIR}"
    RENAME "${package}Config.cmake"
    COMPONENT speedgun-ng_Development
)

install(
    FILES "${PROJECT_BINARY_DIR}/${package}ConfigVersion.cmake"
    DESTINATION "${speedgun-ng_INSTALL_CMAKEDIR}"
    COMPONENT speedgun-ng_Development
)

install(
    EXPORT speedgun-ngTargets
    NAMESPACE speedgun-ng::
    DESTINATION "${speedgun-ng_INSTALL_CMAKEDIR}"
    COMPONENT speedgun-ng_Development
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
