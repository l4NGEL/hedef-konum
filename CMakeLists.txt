cmake_minimum_required(VERSION 3.20)
project(hedef_konum LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
  add_compile_options(/W4 /permissive- /utf-8)
  add_compile_definitions(_USE_MATH_DEFINES)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Çekirdek kütüphane: dış bağımlılık yok, vektör/matris işlerini kendimiz yazıyoruz.
add_library(hk_core
  src/geodezi.cpp
  src/donus.cpp
  src/kamera.cpp
  src/geolokasyon.cpp
  src/dem.cpp
  src/belirsizlik.cpp
  src/senaryo.cpp
  src/harita.cpp
)
target_include_directories(hk_core PUBLIC include)

add_executable(hk_senaryo app/hk_senaryo.cpp)
target_link_libraries(hk_senaryo PRIVATE hk_core)
add_executable(hk_coz app/hk_coz.cpp)
target_link_libraries(hk_coz PRIVATE hk_core)

# Testler (GoogleTest)
option(HK_TESTS "Testleri derle" ON)
if(HK_TESTS)
  include(FetchContent)
  FetchContent_Declare(googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  enable_testing()
  add_executable(hk_tests
    tests/test_geodezi.cpp
    tests/test_donus.cpp
    tests/test_kamera.cpp
    tests/test_geolokasyon.cpp
    tests/test_dem.cpp
    tests/test_belirsizlik.cpp
    tests/test_senaryo.cpp
  )
  target_link_libraries(hk_tests PRIVATE hk_core GTest::gtest_main)
  include(GoogleTest)
  gtest_discover_tests(hk_tests)
endif()
