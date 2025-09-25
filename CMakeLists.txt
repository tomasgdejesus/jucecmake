cmake_minimum_required(VERSION 3.22)
project(juceplugin VERSION 0.1.0 LANGUAGES CXX)

# Download CPM.cmake if not found
if(NOT EXISTS "${CMAKE_BINARY_DIR}/cmake/CPM.cmake")
  file(DOWNLOAD
    https://github.com/cpm-cmake/CPM.cmake/releases/latest/download/CPM.cmake
    "${CMAKE_BINARY_DIR}/cmake/CPM.cmake"
  )
endif()

include("${CMAKE_BINARY_DIR}/cmake/CPM.cmake")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

CPMAddPackage(
  NAME GoogleTest
  GITHUB_REPOSITORY google/googletest
  GIT_TAG v1.17.0
  OPTIONS "INSTALL_GTEST OFF" "gtest_force_shared_crt"
)
enable_testing()

# WebView2 required for Windows
if(WIN32)
  CPMAddPackage(
    NAME WebView2
    URL https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/1.0.3240.44
    DOWNLOAD_ONLY TRUE
  )

  # Set WebView2 for JUCE
  set(WebView2_root_dir "${WebView2_SOURCE_DIR}")
  set(JUCE_WEBVIEW2_PACKAGE_LOCATION "${WebView2_SOURCE_DIR}")
endif()

CPMAddPackage(
  NAME juce
  GITHUB_REPOSITORY juce-framework/JUCE
  GIT_TAG 8.0.7
)

CPMAddPackage(
  NAME luajit
  GITHUB_REPOSITORY WohlSoft/LuaJIT
  GIT_TAG 3e784be65b8fdebec8ae64fdc39547ecc6c7a646
)

add_subdirectory(src/cpp)
add_subdirectory(tests)