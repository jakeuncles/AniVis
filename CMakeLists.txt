# AniVis for Daz Studio 6 (Qt 6.10, Daz Studio 6 SDK).
#
#   cmake -S . -B build -G "Visual Studio 18 2026" -A x64 ^
#     -DDAZ_SDK_DIR="C:/path/to/Daz Studio 6 SDK" ^
#     -DCMAKE_PREFIX_PATH="C:/Qt/6.10.3/msvc2022_64"
#   cmake --build build --config Release
#
# Result: build/Release/dsp_anivis.dll -> copy into <DAZStudio6>/plugins.
cmake_minimum_required(VERSION 3.21)
project(anivis VERSION 2.3.2 LANGUAGES CXX)

set(DAZ_SDK_DIR "" CACHE PATH "Root folder of the Daz Studio 6 SDK (contains include/ and x64/lib/)")
if(NOT DAZ_SDK_DIR)
  message(FATAL_ERROR "Set DAZ_SDK_DIR to the Daz Studio 6 SDK folder.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS Core Gui Widgets)

find_path(DAZ_SDK_INCLUDE_DIR dzapp.h
  PATHS "${DAZ_SDK_DIR}/include" "${DAZ_SDK_DIR}"
  NO_DEFAULT_PATH REQUIRED)

find_library(DAZ_MEMORY_LIBRARY NAMES dzsdkmemory
  PATHS "${DAZ_SDK_DIR}/x64/lib" "${DAZ_SDK_DIR}/lib/x64" "${DAZ_SDK_DIR}/lib"
  NO_DEFAULT_PATH REQUIRED)

find_library(DAZ_CORE_LIBRARY NAMES dzcore
  PATHS
    "${DAZ_SDK_DIR}/x64/lib"
    "${DAZ_SDK_DIR}/lib/x64"
    "${DAZ_SDK_DIR}/lib/Mac64"
    "${DAZ_SDK_DIR}/lib"
  NO_DEFAULT_PATH REQUIRED)

add_library(anivis MODULE
  pluginmain.cpp
  anivisaction.cpp
  anivisaction.h
  wlsAnivisWatcher.cpp
  wlsAnivisWatcher.h
  version.h
  anivisguids.h
  anivispane.cpp
  anivispane.h
  # new/delete from the same memory pool as Daz Studio (it deletes the
  # properties the plugin creates)
  "${DAZ_SDK_INCLUDE_DIR}/dzsdkmemorydefines.cpp"
)

target_include_directories(anivis PRIVATE "${DAZ_SDK_INCLUDE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(anivis PRIVATE "${DAZ_CORE_LIBRARY}" "${DAZ_MEMORY_LIBRARY}" Qt6::Core Qt6::Gui Qt6::Widgets)

if(WIN32)
  target_compile_definitions(anivis PRIVATE UNICODE _UNICODE NOMINMAX)
  # Daz looks up getSDKVersion / getPluginDefinition by their plain names.
  target_sources(anivis PRIVATE anivis.def AniVis.rc)
  # Daz Studio on Windows only loads plugin DLLs named dsp_*.dll.
  set_target_properties(anivis PROPERTIES PREFIX "dsp_" SUFFIX ".dll")
elseif(APPLE)
  set_target_properties(anivis PROPERTIES PREFIX "" SUFFIX ".dylib")
  target_link_options(anivis PRIVATE -undefined dynamic_lookup)
endif()
