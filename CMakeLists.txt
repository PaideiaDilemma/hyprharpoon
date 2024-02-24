cmake_minimum_required(VERSION 3.19)
project(hyprharpoon VERSION 0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)

file(GLOB HYPRHARPOON_SRC *.cpp)

add_library(${PROJECT_NAME} SHARED ${HYPRHARPOON_SRC})

find_package(PkgConfig REQUIRED)
pkg_search_module(HYPRLAND REQUIRED hyprland)
pkg_search_module(LIBDRM REQUIRED hyprland)
pkg_search_module(PIXMAN REQUIRED pixman-1)
pkg_search_module(PANGOCAIRO REQUIRED pangocairo)

target_link_libraries(${PROJECT_NAME} PRIVATE ${HYPRLAND_LIBRARIES} ${LIBDRM_LIBRARIES} ${PIXMAN_LIBRARIES} ${PANGOCAIRO_LIBRARIES})

install(TARGETS ${PROJECT_NAME} DESTINATION lib)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
