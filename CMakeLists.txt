cmake_minimum_required(VERSION 3.16)

project(YLog LANGUAGES CXX)

# ---- C++ standard ----
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Helpful defaults
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---- Build options ----
option(YLOG_BUILD_TEST "Build YLog test executable" ON)

# ---- fmt (bundled) ----
# This repo vendors fmt sources under 3rdparty/.
# We compile fmt as a static library rather than relying on the prebuilt libfmt.a,
# so it works across different compilers / platforms.
add_library(fmt STATIC)
add_library(fmt::fmt ALIAS fmt)

# Use the classic translation units (portable, works on GCC/Clang/MSVC).
# NOTE: 3rdparty/fmt.cc is a C++20 module TU; we intentionally do NOT use it here.
target_sources(fmt PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/format.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/os.cc
)

target_include_directories(fmt PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty
)

# fmt is a normal C++ library; nothing special on MSVC is required here.
# (If you want to reduce warnings, you can add MSVC-specific flags.)
if (MSVC)
  target_compile_definitions(fmt PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# ---- YLog test ----
if (YLOG_BUILD_TEST)
  add_executable(ylog_test ${CMAKE_CURRENT_SOURCE_DIR}/test.cpp)

  # This project is header-only besides the test, so just add include path.
  target_include_directories(ylog_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

  target_link_libraries(ylog_test PRIVATE fmt::fmt)

  # Thread support (needed by AsyncWorker / std::thread usage)
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
  target_link_libraries(ylog_test PRIVATE Threads::Threads)
endif()
