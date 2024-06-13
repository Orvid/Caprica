CMAKE_MINIMUM_REQUIRED(VERSION 3.15 FATAL_ERROR)
cmake_policy(SET CMP0048 NEW) # VERSION variables
cmake_policy(SET CMP0091 NEW) # CMAKE_MSVC_RUNTIME_LIBRARY

# package information
set(PACKAGE_NAME      "Caprica")
set(PACKAGE_VERSION   "0.0.1")
set(PACKAGE_STRING    "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_TARNAME   "${PACKAGE_NAME}-${PACKAGE_VERSION}")
set(PACKAGE_BUGREPORT "https://github.com/Orvid/Caprica/issues")

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "")
endif()

project(${PACKAGE_NAME} VERSION ${PACKAGE_VERSION} LANGUAGES CXX)
include(GNUInstallDirs)


add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING)

option(CAPRICA_STATIC_LIBRARY "Build Caprica as a static library" OFF)
option(CAPRICA_USE_STATIC_RUNTIME "Compile Caprica with static runtime" OFF)

set(CMAKE_CXX_STANDARD 23)


# Automatically create source_group directives for the sources passed in.
function(auto_source_group rootName rootDir)
  file(TO_CMAKE_PATH "${rootDir}" rootDir)
  string(LENGTH "${rootDir}" rootDirLength)
  set(sourceGroups)
  foreach (fil ${ARGN})
    file(TO_CMAKE_PATH "${fil}" filePath)
    string(FIND "${filePath}" "/" rIdx REVERSE)
    if (rIdx EQUAL -1)
      message(FATAL_ERROR "Unable to locate the final forward slash in '${filePath}'!")
    endif()
    string(SUBSTRING "${filePath}" 0 ${rIdx} filePath)

    string(LENGTH "${filePath}" filePathLength)
    string(FIND "${filePath}" "${rootDir}" rIdx)
    if (NOT rIdx EQUAL 0)
      continue()
    endif()
    math(EXPR filePathLength "${filePathLength} - ${rootDirLength}")
    string(SUBSTRING "${filePath}" ${rootDirLength} ${filePathLength} fileGroup)

    string(REPLACE "/" "\\" fileGroup "${fileGroup}")
    set(fileGroup "\\${rootName}${fileGroup}")

    list(FIND sourceGroups "${fileGroup}" rIdx)
    if (rIdx EQUAL -1)
      list(APPEND sourceGroups "${fileGroup}")
      source_group("${fileGroup}" REGULAR_EXPRESSION "${filePath}/[^/.]+.(c|cpp|h|hpp)$")
    endif()
  endforeach()
endfunction()


if (CAPRICA_USE_STATIC_RUNTIME)
  if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set(MSVC_RUNTIME_LIBRARY ${CMAKE_MSVC_RUNTIME_LIBRARY})
  else()
    set(USE_STATIC_RUNTIME ON)
  endif()
endif()




set(CAPRICA_TARGET_NAME                ${PROJECT_NAME})
set(CAPRICA_CONFIG_INSTALL_DIR         "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}" CACHE INTERNAL "")
set(CAPRICA_INCLUDE_INSTALL_DIR        "${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}" CACHE INTERNAL "")
set(CAPRICA_TARGETS_EXPORT_NAME        "${PROJECT_NAME}-targets")
set(CAPRICA_CMAKE_CONFIG_TEMPLATE      "${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/cmake/Config.cmake.in")
set(CAPRICA_CMAKE_CONFIG_DIR           "${CMAKE_CURRENT_BINARY_DIR}")
set(CAPRICA_CMAKE_VERSION_CONFIG_FILE  "${CAPRICA_CMAKE_CONFIG_DIR}/${PROJECT_NAME}ConfigVersion.cmake")
set(CAPRICA_CMAKE_PROJECT_CONFIG_FILE  "${CAPRICA_CMAKE_CONFIG_DIR}/${PROJECT_NAME}Config.cmake")
set(CAPRICA_CMAKE_PROJECT_TARGETS_FILE "${CAPRICA_CMAKE_CONFIG_DIR}/${PROJECT_NAME}-targets.cmake")


if (VCPKG_TARGET_TRIPLET MATCHES "static")
  if(NOT MSVC)
    set(Boost_USE_STATIC_LIBS ON)
    set(Boost_USE_STATIC_RUNTIME ON)
  endif()
endif()

if(CAPRICA_STATIC_LIBRARY)
  
  file(GLOB HEADER_FILES "Caprica/**/*.h")
  file(GLOB SOURCE_FILES "Caprica/**/*.cpp")

  add_library("${PROJECT_NAME}" STATIC ${HEADER_FILES} ${SOURCE_FILES})
  find_package(Boost COMPONENTS container REQUIRED)
  find_package(fmt REQUIRED)
  find_package(pugixml CONFIG REQUIRED)
  target_link_libraries(${PROJECT_NAME} PRIVATE Boost::container fmt::fmt)
  target_link_libraries(${PROJECT_NAME} PRIVATE pugixml pugixml::static pugixml::pugixml)

  target_include_directories(
    "${PROJECT_NAME}"
    PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/Caprica>
    $<INSTALL_INTERFACE:${CAPRICA_INCLUDE_INSTALL_DIR}>
  )
  include(CMakePackageConfigHelpers)
  write_basic_package_version_file(
      ${CAPRICA_CMAKE_VERSION_CONFIG_FILE} COMPATIBILITY SameMinorVersion
  )
  configure_package_config_file(
    ${CAPRICA_CMAKE_CONFIG_TEMPLATE}
    "${CAPRICA_CMAKE_PROJECT_CONFIG_FILE}" 
    INSTALL_DESTINATION ${CAPRICA_CONFIG_INSTALL_DIR}
  )

  install(
    TARGETS "${CAPRICA_TARGET_NAME}"
    EXPORT "${CAPRICA_TARGETS_EXPORT_NAME}"
  )

  install(
    EXPORT "${CAPRICA_TARGETS_EXPORT_NAME}"
    NAMESPACE "${CAPRICA_TARGET_NAME}::"
    DESTINATION "${CAPRICA_CONFIG_INSTALL_DIR}"
  )
  install(
    FILES ${HEADER_FILES}
    DESTINATION "${CAPRICA_INCLUDE_INSTALL_DIR}"
  )
  install(FILES ${CAPRICA_CMAKE_VERSION_CONFIG_FILE} ${CAPRICA_CMAKE_PROJECT_CONFIG_FILE}
  DESTINATION ${CAPRICA_CONFIG_INSTALL_DIR})
else()
  find_package(Boost COMPONENTS filesystem program_options container REQUIRED)
  find_package(fmt REQUIRED)
  find_package(pugixml CONFIG REQUIRED)
  include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/Caprica
    ${Boost_INCLUDE_DIRS}
  )
  add_subdirectory(Caprica)
  add_dependencies(${PROJECT_NAME} Caprica)
  target_link_libraries(${PROJECT_NAME} PRIVATE Boost::filesystem Boost::program_options Boost::container fmt::fmt)
  target_link_libraries(${PROJECT_NAME} PRIVATE pugixml pugixml::static pugixml::pugixml)

  install(
    TARGETS Caprica
  )
endif()