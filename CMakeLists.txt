cmake_minimum_required(VERSION 3.14)
project(NetDebugHost)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出目录（可执行文件、静态库、动态库等）
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 子目录
set(LIBXR_LOG_LEVEL 3)
add_subdirectory(libxr)
add_subdirectory(Modules)

add_executable(${PROJECT_NAME} User/main.cpp)

target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC xr
)

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC $<TARGET_PROPERTY:xr,INTERFACE_INCLUDE_DIRECTORIES>
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/User
)

find_library(WPA_CLIENT_LIB wpa_client REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUDEV REQUIRED libudev)

target_include_directories(${PROJECT_NAME} PRIVATE ${LIBUDEV_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBUDEV_LIBRARIES})

include(Modules/CMakeLists.txt)
