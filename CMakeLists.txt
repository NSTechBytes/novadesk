cmake_minimum_required(VERSION 3.20)
project(Novadesk LANGUAGES C CXX)

if(NOT WIN32)
  message(FATAL_ERROR "Novadesk currently builds on Windows only.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(NOVADESK_CORE_ROOT "${CMAKE_SOURCE_DIR}/src/apps/novadesk/core")
set(NOVADESK_THIRD_PARTY_ROOT "${CMAKE_SOURCE_DIR}/src/third_party")
if(EXISTS "${CMAKE_SOURCE_DIR}/Widgets")
  set(NOVADESK_WIDGETS_ROOT "${CMAKE_SOURCE_DIR}/Widgets")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/src/Widgets")
  set(NOVADESK_WIDGETS_ROOT "${CMAKE_SOURCE_DIR}/src/Widgets")
else()
  set(NOVADESK_WIDGETS_ROOT "")
  message(WARNING "Widgets folder not found (expected Widgets/ or src/Widgets/).")
endif()

file(GLOB_RECURSE NOVADESK_CORE_CPP CONFIGURE_DEPENDS
  "${NOVADESK_CORE_ROOT}/*.cpp"
)

set(NOVADESK_THIRD_PARTY_C
  "${NOVADESK_THIRD_PARTY_ROOT}/duktape/duktape.c"
  "${NOVADESK_THIRD_PARTY_ROOT}/kiss_fft130/kiss_fft.c"
  "${NOVADESK_THIRD_PARTY_ROOT}/kiss_fft130/kiss_fftr.c"
)

set(NOVADESK_APP_SOURCES
  "${CMAKE_SOURCE_DIR}/src/apps/novadesk/core/domain/Novadesk.cpp"
  "${CMAKE_SOURCE_DIR}/src/apps/novadesk/Novadesk.rc"
)

add_executable(novadesk WIN32
  ${NOVADESK_APP_SOURCES}
  ${NOVADESK_CORE_CPP}
  ${NOVADESK_THIRD_PARTY_C}
)

target_include_directories(novadesk PRIVATE
  "${NOVADESK_CORE_ROOT}/domain"
  "${NOVADESK_CORE_ROOT}/render"
  "${NOVADESK_CORE_ROOT}/platform/win32"
  "${NOVADESK_CORE_ROOT}/services"
  "${NOVADESK_CORE_ROOT}/parsing"
  "${NOVADESK_CORE_ROOT}/scripting/duktape"
  "${NOVADESK_CORE_ROOT}/api/js"
  "${NOVADESK_CORE_ROOT}/shared"
  "${NOVADESK_THIRD_PARTY_ROOT}/duktape"
  "${NOVADESK_THIRD_PARTY_ROOT}/json"
  "${NOVADESK_THIRD_PARTY_ROOT}/kiss_fft130"
  "${NOVADESK_THIRD_PARTY_ROOT}/nanosvg"
)

target_compile_definitions(novadesk PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN)
target_link_libraries(novadesk PRIVATE
  d2d1
  dwrite
  windowscodecs
  winmm
  iphlpapi
  wininet
  shlwapi
  shell32
  comctl32
  ole32
  oleaut32
  uuid
  user32
  gdi32
  advapi32
  powrprof
)

# Copy Widgets folder to target output, matching existing VS post-build behavior.
if(NOVADESK_WIDGETS_ROOT)
  add_custom_command(TARGET novadesk POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${NOVADESK_WIDGETS_ROOT}" "$<TARGET_FILE_DIR:novadesk>/Widgets"
  )
endif()

add_executable(nwm
  "${CMAKE_SOURCE_DIR}/src/apps/nwm/src/main.cpp"
  "${CMAKE_SOURCE_DIR}/src/apps/nwm/src/rescle.cc"
  "${CMAKE_SOURCE_DIR}/src/apps/nwm/nwm.rc"
)
target_include_directories(nwm PRIVATE "${CMAKE_SOURCE_DIR}/src/apps/nwm/src")
target_compile_definitions(nwm PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN)
target_link_libraries(nwm PRIVATE
  user32
  shell32
  shlwapi
  advapi32
  version
)

add_executable(installer_stub WIN32
  "${CMAKE_SOURCE_DIR}/src/apps/installer_stub/src/installer_stub.cpp"
  "${CMAKE_SOURCE_DIR}/src/apps/installer_stub/installer_stub.rc"
)
target_include_directories(installer_stub PRIVATE
  "${CMAKE_SOURCE_DIR}/src/apps/nwm/src"
  "${NOVADESK_THIRD_PARTY_ROOT}/json"
)
target_compile_definitions(installer_stub PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN)
target_link_libraries(installer_stub PRIVATE
  user32
  shell32
  shlwapi
  advapi32
  version
)
