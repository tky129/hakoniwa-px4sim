set(HAKO_CMAKE_VERSION 3.5.1)
cmake_minimum_required(VERSION ${HAKO_CMAKE_VERSION})

project(drone_physics
    VERSION 1.0.0
    DESCRIPTION "matlab interface project"
    LANGUAGES C CXX
)

if (DEFINED HAKO_CLIENT_OPTION_FILEPATH)
    include(${HAKO_CLIENT_OPTION_FILEPATH})
else()
    include(../cmake-options/default-cmake-options.cmake)
endif()

add_executable(acctest acctest.cpp)
target_include_directories(acctest PUBLIC ../drone_physics)
target_link_libraries(acctest PRIVATE drone_physics_c)
add_library(drone_physics_c STATIC IMPORTED)
set_property(TARGET drone_physics_c PROPERTY IMPORTED_LOCATION "../drone_physics/libdrone_physics_c.a")

enable_testing()
add_test(NAME test COMMAND ./acctest)
add_custom_target(vtest COMMAND ${CMAKE_CTEST_COMMAND} --verbose)

