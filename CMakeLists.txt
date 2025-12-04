cmake_minimum_required(VERSION 3.16)
project(lab_04)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Скачиваем библиотеку с тестами
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.17.0
)
FetchContent_MakeAvailable(googletest)

add_executable(lab_04 main.cpp)

enable_testing()

add_executable(tests test.cpp)
target_link_libraries(tests gtest_main)

target_compile_options(lab_04 PRIVATE -Wall -Wextra -pedantic)
target_compile_options(tests PRIVATE -Wall -Wextra -pedantic)

add_test(NAME Tests COMMAND tests)