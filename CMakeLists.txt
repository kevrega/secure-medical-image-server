cmake_minimum_required(VERSION 3.16)

project(SecureMedicalImageServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --------------------------------------------------
# Core library
# --------------------------------------------------

add_library(medical_core
    src/Patient.cpp
    src/User.cpp
    src/Study.cpp
    src/FileManager.cpp
)

target_include_directories(medical_core
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_compile_options(medical_core
    PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

# --------------------------------------------------
# Main application
# --------------------------------------------------

add_executable(med
    src/main.cpp
)

target_link_libraries(med
    PRIVATE
    medical_core
)

target_compile_options(med
    PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

# --------------------------------------------------
# GoogleTest
# --------------------------------------------------

include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
)

FetchContent_MakeAvailable(googletest)

enable_testing()

# --------------------------------------------------
# Unit tests
# --------------------------------------------------

add_executable(unit_tests
    tests/test_patient.cpp
    tests/test_user.cpp
    tests/test_study.cpp
    tests/test_storage.cpp
)

target_link_libraries(unit_tests
    PRIVATE
    medical_core
    GTest::gtest_main
)

target_compile_options(unit_tests
    PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

include(GoogleTest)

gtest_discover_tests(unit_tests)