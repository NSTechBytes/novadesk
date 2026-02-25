cmake_minimum_required(VERSION 3.20)

project(novadesk_runner LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(APP_SRC_DIR "${CMAKE_SOURCE_DIR}/src/apps/novadesk")
set(THIRD_PARTY_DIR "${CMAKE_SOURCE_DIR}/src/third_party")
set(QUICKJS_DIR "${THIRD_PARTY_DIR}/quick-js")

file(GLOB_RECURSE NOVADESK_APP_SOURCES CONFIGURE_DEPENDS
    "${APP_SRC_DIR}/*.cpp"
)

add_executable(novadesk WIN32
    ${NOVADESK_APP_SOURCES}
    "${APP_SRC_DIR}/Novadesk.rc"
    "${QUICKJS_DIR}/quickjs.c"
    "${QUICKJS_DIR}/quickjs-libc.c"
    "${QUICKJS_DIR}/libregexp.c"
    "${QUICKJS_DIR}/libunicode.c"
    "${QUICKJS_DIR}/dtoa.c"
)

target_compile_definitions(novadesk PRIVATE UNICODE _UNICODE)

target_include_directories(novadesk PRIVATE
    "${APP_SRC_DIR}"
    "${APP_SRC_DIR}/domain"
    "${APP_SRC_DIR}/shared"
    "${APP_SRC_DIR}/render"
    "${APP_SRC_DIR}/scripting/quickjs"
    "${APP_SRC_DIR}/scripting/quickjs/parser"
    "${QUICKJS_DIR}"
)

target_link_libraries(novadesk PRIVATE
    user32
    gdi32
    opengl32
    winmm
    shell32
    comctl32
    d2d1
    dwrite
    windowscodecs
    shlwapi
    version
)
