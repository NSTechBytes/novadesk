cmake_minimum_required(VERSION 3.20)

project(novadesk_runner LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(APP_SRC_DIR "${CMAKE_SOURCE_DIR}/src/apps/novadesk")
set(APP_ASSETS_DIR "${CMAKE_SOURCE_DIR}/src/Widgets/builtin")
set(THIRD_PARTY_DIR "${CMAKE_SOURCE_DIR}/src/third_party")
set(QUICKJS_DIR "${THIRD_PARTY_DIR}/quick-js")
set(RAPIDJSON_DIR "${THIRD_PARTY_DIR}/rapidjson/include")

add_executable(novadesk
    "${APP_SRC_DIR}/main.cpp"
    "${APP_SRC_DIR}/domain/Widget.cpp"
    "${APP_SRC_DIR}/shared/Utils.cpp"
    "${APP_SRC_DIR}/scripting/quickjs/ModuleSystem.cpp"
    "${APP_SRC_DIR}/scripting/quickjs/NovadeskModule.cpp"
    "${APP_SRC_DIR}/scripting/quickjs/parser/PropertyParser.cpp"
    "${QUICKJS_DIR}/quickjs.c"
    "${QUICKJS_DIR}/libregexp.c"
    "${QUICKJS_DIR}/libunicode.c"
    "${QUICKJS_DIR}/dtoa.c"
)

target_include_directories(novadesk PRIVATE
    "${QUICKJS_DIR}"
    "${RAPIDJSON_DIR}"
)

if(WIN32)
    target_include_directories(novadesk PRIVATE "${THIRD_PARTY_DIR}/RGFW/win/include")
    target_link_libraries(novadesk PRIVATE user32 gdi32 opengl32 winmm shell32)
else()
    target_include_directories(novadesk PRIVATE "${THIRD_PARTY_DIR}/RGFW/linux/include")
    target_link_libraries(novadesk PRIVATE X11 GL Xrandr m pthread dl)
endif()

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
