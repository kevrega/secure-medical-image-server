cmake_minimum_required(VERSION 3.16)

project(SecureMedicalImageServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(med
    src/main.cpp
    src/Patient.cpp
    src/Study.cpp
    src/User.cpp
    src/FileManager.cpp
)

target_include_directories(med PRIVATE include)