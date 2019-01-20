cmake_minimum_required(VERSION 3.5)

project(ncursesFM VERSION 3.1 LANGUAGES C)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(GNUInstallDirs)
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(Libmagic REQUIRED)

# Options
set(NCURSESFM_CONFDIR "${CMAKE_INSTALL_FULL_SYSCONFDIR}/default"
    CACHE FILEPATH "Path for config file")

# Create program target
file(GLOB_RECURSE SOURCES src/*.c)
add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE
                           "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_compile_definitions(${PROJECT_NAME} PRIVATE
    -D_GNU_SOURCE
    -DVERSION="${PROJECT_VERSION}"
    -DCONFDIR="${NCURSESFM_CONFDIR}"
    -DBINDIR="${CMAKE_INSTALL_BINDIR}"
    -DLOCALEDIR="${CMAKE_INSTALL_LOCALEDIR}"
)
set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 99)

# Required dependencies
pkg_check_modules(REQ_LIBS REQUIRED libconfig libarchive ncursesw libudev)
pkg_search_module(LOGIN_LIBS REQUIRED libelogind libsystemd>=221)

if (ENABLE_CUPS)
    find_package(Cups REQUIRED)
    message(STATUS "Libups support enabled")
    target_compile_definitions(${PROJECT_NAME} PRIVATE LIBCUPS_PRESENT)
endif()

if (ENABLE_NOTIFY)
    pkg_check_modules(OTHER_LIBS REQUIRED libnotify)
    message(STATUS "Libnotify support enabled")
    target_compile_definitions(${PROJECT_NAME} PRIVATE LIBNOTIFY_PRESENT)
endif()

target_link_libraries(${PROJECT_NAME}
                      m
                      ${REQ_LIBS_LIBRARIES}
                      ${OTHER_LIBS_LIBRARIES}
                      ${LOGIN_LIBS_LIBRARIES}
                      ${CMAKE_THREAD_LIBS_INIT}
                      ${LIBMAGIC_LIBRARIES}
                      ${CUPS_LIBRARIES}
)
target_include_directories(${PROJECT_NAME} PRIVATE
                            ${REQ_LIBS_INCLUDE_DIRS}
                            ${OTHER_LIBS_INCLUDE_DIRS}
                            ${LOGIN_LIBS_INCLUDE_DIRS}
                            ${LIBMAGIC_INCLUDES}
                            ${CUPS_INCLUDES}
)
list(APPEND COMBINED_LDFLAGS ${REQ_LIBS_LDFLAGS})
list(APPEND COMBINED_LDFLAGS ${LOGIN_LIBS_LDFLAGS})

# Convert ld flag list from list to space separated string.
string(REPLACE ";" " " COMBINED_LDFLAGS "${COMBINED_LDFLAGS}")

# Set the LDFLAGS target property
set_target_properties(
    ${PROJECT_NAME} PROPERTIES
    LINK_FLAGS "${COMBINED_LDFLAGS}"
)

add_subdirectory(msg)

# Installation of targets (must be before file configuration to work)
install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

# Configure files with install paths
set(EXTRA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Script")

# Installation of files
pkg_get_variable(COMPLETIONS_DIR bash-completion completionsdir)

install(FILES ${EXTRA_DIR}/${PROJECT_NAME}.conf
        DESTINATION ${NCURSESFM_CONFDIR})

if (COMPLETIONS_DIR)
    install(FILES ${EXTRA_DIR}/${PROJECT_NAME}
            DESTINATION ${COMPLETIONS_DIR})
endif()
