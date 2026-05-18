cmake_minimum_required(VERSION 3.13)

project(magicseteditor VERSION 2.6.0 LANGUAGES CXX)
# ------------------------------------------------------------------------------
# CPack RPM packaging
# ------------------------------------------------------------------------------

set(CPACK_PACKAGE_NAME "magicseteditor")
set(CPACK_PACKAGE_VENDOR "Continuum Universes Studio")
set(CPACK_PACKAGE_CONTACT "Christian Daniel Jensen")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Magic Set Editor")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")

set(CPACK_GENERATOR "RPM")

set(CPACK_RPM_PACKAGE_LICENSE "GPL-2.0-or-later")
set(CPACK_RPM_PACKAGE_GROUP "Amusements/Games")

set(CPACK_RPM_PACKAGE_REQUIRES
    "wxWidgets >= 3.2, boost-regex, hunspell"
)

set(CPACK_PACKAGE_FILE_NAME
    "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_PROCESSOR}"
)

include(CPack)
# ------------------------------------------------------------------------------
# Options
# ------------------------------------------------------------------------------

option(BUILD_APPLE_BUNDLE "Build magicseteditor as a macOS application bundle" OFF)

# ------------------------------------------------------------------------------
# Global language settings
# ------------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Portable Linux runtime lookup:
# Allows bundled libraries beside the executable in ./lib
if(UNIX AND NOT APPLE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
    set(CMAKE_INSTALL_RPATH "$ORIGIN/lib")
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

# ------------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------------

find_package(wxWidgets 3.3.1 REQUIRED COMPONENTS core base net html)
include(${wxWidgets_USE_FILE})

find_package(Boost REQUIRED COMPONENTS regex)
find_package(PkgConfig REQUIRED)
set(CPACK_RPM_PACKAGE_REQUIRES
    "wxwidgets331 >= 3.3.1, boost-regex, hunspell"
)
# Hunspell
pkg_check_modules(HUNSPELL hunspell)

if(NOT HUNSPELL_FOUND)
    message(STATUS "Cannot find Hunspell via pkg-config, checking directly...")

    find_path(HUNSPELL_INCLUDE_DIRS
        NAMES hunspell.hxx
        PATH_SUFFIXES hunspell
    )

    find_library(HUNSPELL_LIBRARIES
        NAMES hunspell hunspell-1.7 libhunspell
    )

    message(STATUS "HUNSPELL_INCLUDE_DIRS = ${HUNSPELL_INCLUDE_DIRS}")
    message(STATUS "HUNSPELL_LIBRARIES = ${HUNSPELL_LIBRARIES}")

    if(NOT HUNSPELL_INCLUDE_DIRS OR NOT HUNSPELL_LIBRARIES)
        message(FATAL_ERROR "Hunspell cannot be found")
    endif()
endif()

message(STATUS "Found Hunspell includes: ${HUNSPELL_INCLUDE_DIRS}")
message(STATUS "Found Hunspell libraries: ${HUNSPELL_LIBRARIES}")

# ------------------------------------------------------------------------------
# Generated config
# ------------------------------------------------------------------------------

configure_file(
    "${PROJECT_SOURCE_DIR}/src/config.hpp.in"
    "${PROJECT_BINARY_DIR}/src/config.hpp"
)

# ------------------------------------------------------------------------------
# Main executable
# ------------------------------------------------------------------------------

file(GLOB_RECURSE MSE_SOURCES CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
)

list(FILTER MSE_SOURCES EXCLUDE REGEX "win32_cli_wrapper\\.cpp$")

add_executable(magicseteditor WIN32)

target_sources(magicseteditor PRIVATE
    ${MSE_SOURCES}
)

target_compile_definitions(magicseteditor PRIVATE
    UNOFFICIAL_BUILD
)

target_include_directories(magicseteditor PRIVATE
    "${PROJECT_BINARY_DIR}/src"
    "${PROJECT_SOURCE_DIR}/src"
    ${Boost_INCLUDE_DIRS}
    ${HUNSPELL_INCLUDE_DIRS}
)

target_link_libraries(magicseteditor PRIVATE
    ${wxWidgets_LIBRARIES}
    ${Boost_LIBRARIES}
    ${HUNSPELL_LIBRARIES}
)

target_precompile_headers(magicseteditor PRIVATE
    "${PROJECT_SOURCE_DIR}/src/util/prec.hpp"
)

if(UNIX AND NOT APPLE)
    set_target_properties(magicseteditor PROPERTIES
        BUILD_RPATH "$ORIGIN/lib"
        INSTALL_RPATH "$ORIGIN/lib"
    )
endif()

if(WIN32)
    target_sources(magicseteditor PRIVATE
        "${PROJECT_SOURCE_DIR}/resource/win32_res.rc"
    )

    target_link_libraries(magicseteditor PRIVATE
        uxtheme
        comctl32
        ole32
        uuid
        gdi32
        comdlg32
        winmm
        shell32
        shlwapi
    )
endif()

# ------------------------------------------------------------------------------
# Compiler warnings
# ------------------------------------------------------------------------------

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(magicseteditor PRIVATE
        -Wall
        -Wextra
        -Wsuggest-override
        -Wno-comment
        -Wno-unused-parameter
    )

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(magicseteditor PRIVATE
            -Wstrict-null-sentinel
        )
    endif()
endif()

# ------------------------------------------------------------------------------
# Windows CLI wrapper
# ------------------------------------------------------------------------------

if(WIN32)
    add_executable(magicseteditor-cli)

    target_sources(magicseteditor-cli PRIVATE
        "${PROJECT_SOURCE_DIR}/src/cli/win32_cli_wrapper.cpp"
    )

    add_custom_command(
        TARGET magicseteditor-cli
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            "$<TARGET_FILE:magicseteditor-cli>"
            "${PROJECT_BINARY_DIR}/magicseteditor.com"
    )
endif()

# ------------------------------------------------------------------------------
# Updater executable
# ------------------------------------------------------------------------------

add_executable(magicseteditor-updater WIN32)

target_sources(magicseteditor-updater PRIVATE
    "${PROJECT_SOURCE_DIR}/src_updater/main.cpp"
)

target_include_directories(magicseteditor-updater PRIVATE
    "${PROJECT_BINARY_DIR}/src"
    "${PROJECT_SOURCE_DIR}/src"
)

target_link_libraries(magicseteditor-updater PRIVATE
    ${wxWidgets_LIBRARIES}
)

if(WIN32)
    target_sources(magicseteditor-updater PRIVATE
        "${PROJECT_SOURCE_DIR}/src_updater/win32_res.rc"
        "${PROJECT_SOURCE_DIR}/src_updater/updater.ico"
    )
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(magicseteditor-updater PRIVATE
        -Wall
        -Wextra
        -Wno-comment
        -Wno-unused-parameter
    )
endif()

# ------------------------------------------------------------------------------
# macOS bundle
# ------------------------------------------------------------------------------

if(BUILD_APPLE_BUNDLE)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
        "${CMAKE_BINARY_DIR}/magicseteditor.app/Contents/PlugIns"
    )

    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
        "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
    )

    set_target_properties(magicseteditor PROPERTIES
        MACOSX_BUNDLE_NAME "magicseteditor"
        MACOSX_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.magicseteditor.gui"
        MACOSX_BUNDLE_ICON_FILE "mse.icns"
        MACOSX_BUNDLE_INFO_PLIST "${PROJECT_SOURCE_DIR}/MacOSXBundleInfo.plist.in"
        MACOSX_BUNDLE TRUE
        RESOURCE "${RESOURCE_FILES}"
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
        XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED NO
        XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT dwarf
        XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN YES
        XCODE_ATTRIBUTE_CLANG_LINK_OBJC_RUNTIME NO
    )

    add_custom_command(
        TARGET magicseteditor
        POST_BUILD
        COMMAND dylibbundler
            -od
            -ns
            -b
            -x "${CMAKE_BINARY_DIR}/magicseteditor.app/Contents/MacOS/magicseteditor"
            -d "${CMAKE_BINARY_DIR}/magicseteditor.app/Contents/libs"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        COMMENT "Changing library search path from absolute to relative..."
    )

    add_custom_command(
        TARGET magicseteditor
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CMAKE_BINARY_DIR}/magicseteditor.app/Contents/Resources"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        COMMENT "Creating Resources directory..."
    )

    file(GLOB MSE_ICON_FILES CONFIGURE_DEPENDS
        "${PROJECT_SOURCE_DIR}/*.icns"
    )

    foreach(MSE_ICON_FILE IN LISTS MSE_ICON_FILES)
        add_custom_command(
            TARGET magicseteditor
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${MSE_ICON_FILE}"
                "${CMAKE_BINARY_DIR}/magicseteditor.app/Contents/Resources/"
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
            COMMENT "Copying icon ${MSE_ICON_FILE}..."
        )
    endforeach()
endif()

# ------------------------------------------------------------------------------
# Static linking on Windows via vcpkg static triplet
# ------------------------------------------------------------------------------

if(DEFINED VCPKG_TARGET_TRIPLET AND VCPKG_TARGET_TRIPLET MATCHES ".*-static")
    message(STATUS "Using static vcpkg triplet: ${VCPKG_TARGET_TRIPLET}")

    if(MSVC)
        foreach(flag_var
            CMAKE_CXX_FLAGS
            CMAKE_CXX_FLAGS_DEBUG
            CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_RELWITHDEBINFO
            CMAKE_C_FLAGS
            CMAKE_C_FLAGS_DEBUG
            CMAKE_C_FLAGS_RELEASE
        )
            if(DEFINED ${flag_var})
                string(REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
            endif()
        endforeach()

        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GL")
        set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /GL")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG")
    endif()

    target_compile_definitions(magicseteditor PRIVATE
        STATIC
        HUNSPELL_STATIC
    )
endif()

# ------------------------------------------------------------------------------
# Visual Studio debugger
# ------------------------------------------------------------------------------

set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    PROPERTY VS_STARTUP_PROJECT magicseteditor
)


# ------------------------------------------------------------------------------
# Install layout for portable Linux builds
# ------------------------------------------------------------------------------

install(TARGETS magicseteditor
    RUNTIME DESTINATION bin
)

# Add these only if these folders exist in your project.
# Uncomment as needed.
#
# install(DIRECTORY data/
#     DESTINATION data
# )
#
# install(DIRECTORY resource/
#     DESTINATION resource
# )

if(UNIX AND NOT APPLE)
    install(CODE [[
        message(STATUS "Linux install created. Bundle non-system shared libraries into: ${CMAKE_INSTALL_PREFIX}/lib")
    ]])
endif()

# ------------------------------------------------------------------------------
# Test suite
# ------------------------------------------------------------------------------

include(test/tests.cmake)
