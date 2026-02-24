cmake_minimum_required(VERSION 3.20)

project(novadesk_runner LANGUAGES C CXX)

if(NOT WIN32)
    message(FATAL_ERROR "This project is configured for Windows only.")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(APP_SRC_DIR "${CMAKE_SOURCE_DIR}/src/apps/novadesk")
set(APP_ASSETS_DIR "${CMAKE_SOURCE_DIR}/src/Widgets/builtin")
set(APP_WIDGETS_DIR "${APP_ASSETS_DIR}/Widgets")
set(THIRD_PARTY_DIR "${CMAKE_SOURCE_DIR}/src/third_party")
set(QUICKJS_DIR "${THIRD_PARTY_DIR}/quick-js")
set(RAPIDJSON_DIR "${THIRD_PARTY_DIR}/rapidjson/include")

file(GLOB_RECURSE NOVADESK_APP_SOURCES CONFIGURE_DEPENDS
    "${APP_SRC_DIR}/*.cpp"
)

add_executable(novadesk
    ${NOVADESK_APP_SOURCES}
    "${APP_SRC_DIR}/Novadesk.rc"
    "${QUICKJS_DIR}/quickjs.c"
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
    "${RAPIDJSON_DIR}"
)

target_include_directories(novadesk PRIVATE "${THIRD_PARTY_DIR}/RGFW/win/include")
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

add_custom_command(TARGET novadesk POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${APP_ASSETS_DIR}/meta.json"
        "$<TARGET_FILE_DIR:novadesk>/meta.json"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${APP_ASSETS_DIR}/main.js"
        "$<TARGET_FILE_DIR:novadesk>/main.js"
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:novadesk>/Widgets"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${APP_WIDGETS_DIR}/index.js"
        "$<TARGET_FILE_DIR:novadesk>/Widgets/index.js"
)

if(WIN32 AND MINGW)
    get_filename_component(MINGW_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)

    set(MINGW_RUNTIME_DLLS
        "${MINGW_BIN_DIR}/libstdc++-6.dll"
        "${MINGW_BIN_DIR}/libwinpthread-1.dll"
    )

    if(EXISTS "${MINGW_BIN_DIR}/libgcc_s_seh-1.dll")
        list(APPEND MINGW_RUNTIME_DLLS "${MINGW_BIN_DIR}/libgcc_s_seh-1.dll")
    elseif(EXISTS "${MINGW_BIN_DIR}/libgcc_s_dw2-1.dll")
        list(APPEND MINGW_RUNTIME_DLLS "${MINGW_BIN_DIR}/libgcc_s_dw2-1.dll")
    elseif(EXISTS "${MINGW_BIN_DIR}/libgcc_s_sjlj-1.dll")
        list(APPEND MINGW_RUNTIME_DLLS "${MINGW_BIN_DIR}/libgcc_s_sjlj-1.dll")
    endif()

    foreach(MINGW_DLL IN LISTS MINGW_RUNTIME_DLLS)
        if(EXISTS "${MINGW_DLL}")
            add_custom_command(TARGET novadesk POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${MINGW_DLL}"
                    "$<TARGET_FILE_DIR:novadesk>"
            )
        endif()
    endforeach()
endif()
