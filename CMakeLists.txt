
cmake_minimum_required(VERSION 3.10)
project(collab_editor C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g")

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK REQUIRED gtk4)
pkg_check_modules(LIBWEBSOCKETS REQUIRED libwebsockets)
pkg_check_modules(SQLITE REQUIRED sqlite3)
pkg_check_modules(JANSSON REQUIRED jansson)
pkg_check_modules(LIBJWT REQUIRED libjwt)

include_directories(
    include
    ${GTK_INCLUDE_DIRS}
    ${LIBWEBSOCKETS_INCLUDE_DIRS}
    ${SQLITE_INCLUDE_DIRS}
    ${JANSSON_INCLUDE_DIRS}
    ${LIBJWT_INCLUDE_DIRS}
)

add_executable(collab_editor
    src/main.c
    src/auth.c
    src/config.c
    src/crdt.c
    src/editor.c
    src/network.c
    src/persistence.c
    src/sync.c
    src/ui.c
    src/utils.c
)

target_link_libraries(collab_editor
    ${GTK_LIBRARIES}
    ${LIBWEBSOCKETS_LIBRARIES}
    ${SQLITE_LIBRARIES}
    ${JANSSON_LIBRARIES}
    ${LIBJWT_LIBRARIES}
    pthread
)
