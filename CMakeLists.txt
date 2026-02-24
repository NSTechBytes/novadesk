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
set(THIRD_PARTY_DIR "${CMAKE_SOURCE_DIR}/src/third_party")
set(QUICKJS_DIR "${THIRD_PARTY_DIR}/quick-js")
set(RAPIDJSON_DIR "${THIRD_PARTY_DIR}/rapidjson/include")

set(NOVADESK_SOURCES
    "${APP_SRC_DIR}/domain/Widget.cpp"
    "${APP_SRC_DIR}/shared/Utils.cpp"
)

set(NOVADESK_RUNTIME_SOURCES
    "${APP_SRC_DIR}/main.cpp"
    "${APP_SRC_DIR}/scripting/quickjs/ModuleSystem.cpp"
    "${APP_SRC_DIR}/scripting/quickjs/NovadeskModule.cpp"
    "${APP_SRC_DIR}/scripting/quickjs/parser/PropertyParser.cpp"
    "${QUICKJS_DIR}/quickjs.c"
    "${QUICKJS_DIR}/libregexp.c"
    "${QUICKJS_DIR}/libunicode.c"
    "${QUICKJS_DIR}/dtoa.c"
)

set(NOVADESK_LEGACY_SOURCES
    "${APP_SRC_DIR}/domain/Novadesk.cpp"
    "${APP_SRC_DIR}/domain/System.cpp"
    "${APP_SRC_DIR}/Novadesk.rc"
)

if(EXISTS "${APP_SRC_DIR}/main.cpp")
    list(APPEND NOVADESK_SOURCES ${NOVADESK_RUNTIME_SOURCES})
elseif(EXISTS "${APP_SRC_DIR}/domain/Novadesk.cpp")
    if(NOT EXISTS "${APP_SRC_DIR}/JSApi/JSApi.h")
        message(FATAL_ERROR
            "Legacy layout detected but required file is missing: "
            "'${APP_SRC_DIR}/JSApi/JSApi.h'. "
            "Restore legacy JSApi files or add '${APP_SRC_DIR}/main.cpp' runtime entrypoint.")
    endif()
    list(APPEND NOVADESK_SOURCES ${NOVADESK_LEGACY_SOURCES})
else()
    message(FATAL_ERROR
        "No supported Novadesk entrypoint found. Expected either "
        "'${APP_SRC_DIR}/main.cpp' (runtime layout) or "
        "'${APP_SRC_DIR}/domain/Novadesk.cpp' (legacy layout).")
endif()

add_executable(novadesk ${NOVADESK_SOURCES})

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
target_link_libraries(novadesk PRIVATE user32 gdi32 opengl32 winmm shell32)

add_custom_command(TARGET novadesk POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${APP_ASSETS_DIR}/meta.json"
        "$<TARGET_FILE_DIR:novadesk>/meta.json"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${APP_ASSETS_DIR}/main.js"
        "$<TARGET_FILE_DIR:novadesk>/main.js"
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
