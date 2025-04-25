cmake_minimum_required(VERSION 3.16)

project(lvgl_simulator VERSION 0.1 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets Quick Qml)

qt_standard_project_setup(REQUIRES 6.8)

set(LV_CONF_DIR ${CMAKE_CURRENT_LIST_DIR})

option(LV_CONF_BUILD_DISABLE_THORVG_INTERNAL "LV_CONF_BUILD_DISABLE_THORVG_INTERNAL" ON)
option(LV_CONF_BUILD_DISABLE_EXAMPLES "LV_CONF_BUILD_DISABLE_EXAMPLES" ON)
option(LV_CONF_BUILD_DISABLE_DEMOS "LV_CONF_BUILD_DISABLE_DEMOS" ON)

add_subdirectory(3rdparty/lvgl lvgl_build)

add_subdirectory(LVGLWidegt)
add_subdirectory(LVGLQML)

# SDL_TEST_LIBRARY OFF
add_subdirectory(3rdparty/SDL sdl3_build)

add_subdirectory(LVGLSDL3)
