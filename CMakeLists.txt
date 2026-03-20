cmake_minimum_required(VERSION 3.16)

project(lvgl_simulator VERSION 0.1 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 COMPONENTS Core Gui Widgets Quick Qml)

set(LV_BUILD_CONF_DIR
        ${CMAKE_CURRENT_LIST_DIR}
        CACHE PATH "Path to lv_conf.h" FORCE)

set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "Disable LVGL demos" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "Disable LVGL examples" FORCE)
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "Disable ThorVG internal" FORCE)

add_subdirectory(3rdparty/lvgl lvgl_build)

include_directories(3rdparty)

if(Qt6_FOUND)
    message(STATUS "Qt6 found, enabling Qt modules")

    qt_standard_project_setup(REQUIRES 6.8)

    add_subdirectory(LVGLWidget)
    add_subdirectory(LVGLQML)

else()
    message(WARNING
            "Qt6 not found. QtWidget/QML modules will NOT be built.\n"
            "\n"
            "To fix this, set Qt6 CMake path, for example:\n"
            "  -DQt6_DIR=/path/to/Qt/6.x.x/gcc_64/lib/cmake/Qt6\n"
            "or:\n"
            "  -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/gcc_64\n"
            "\n"
            "Then re-run CMake."
    )
endif()

set(SDL_TEST_LIBRARY OFF CACHE BOOL "Disable SDL3_test library" FORCE)

add_subdirectory(3rdparty/SDL sdl3_build)
add_subdirectory(LVGLSDL3)
