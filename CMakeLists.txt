cmake_minimum_required(VERSION 3.10)
project(strava_art)
set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GDAL REQUIRED gdal)
find_package(Boost REQUIRED COMPONENTS serialization)
find_package(Threads REQUIRED)

# FMM headers & libs
find_path(FMM_INCLUDE_DIR fmm/fmm.hpp PATHS /usr/local/include)
find_library(FMM_LIB fmm PATHS /usr/local/lib)

add_executable(strava_art strava_art.cpp)
target_include_directories(strava_art PRIVATE ${FMM_INCLUDE_DIR} ${GDAL_INCLUDE_DIRS})
target_link_libraries(strava_art
        ${FMM_LIB}
        ${GDAL_LIBRARIES}
        ${Boost_LIBRARIES}
        Threads::Threads)
