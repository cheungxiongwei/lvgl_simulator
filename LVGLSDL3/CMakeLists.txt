add_executable(LVGLSDL3
    main.cpp
    main.h
)

# Graphics demo selection switches.
# WARNING: These options are mutually exclusive in normal usage.
# Enable only one Graphics version to avoid conflicting demo builds.

option(BUILD_GRAPHICS_V1_DEMOS "Enable building demos for Graphics V1" OFF)
option(BUILD_GRAPHICS_V2_DEMOS "Enable building demos for Graphics V2" OFF)
option(BUILD_GRAPHICS_V3_DEMOS "Enable building demos for Graphics V3" OFF)

# test
set(BUILD_GRAPHICS_V1_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_GRAPHICS_V2_DEMOS ON CACHE BOOL "" FORCE)
set(BUILD_GRAPHICS_V3_DEMOS OFF CACHE BOOL "" FORCE)

if(BUILD_GRAPHICS_V1_DEMOS)
    target_sources(LVGLSDL3
        PRIVATE
            demos/graphics/v1/src/Graphics.cpp
    )
    target_compile_definitions(LVGLSDL3
        PRIVATE ENABLE_GRAPHICS_V1_DEMOS
    ) 
    target_include_directories(LVGLSDL3
        PRIVATE demos/graphics/v1/include demos/graphics/v1/src
    )
endif()

if(BUILD_GRAPHICS_V2_DEMOS)
    target_sources(LVGLSDL3
        PRIVATE
            demos/graphics/v2/src/Graphics.cpp
    )
    target_compile_definitions(LVGLSDL3
        PRIVATE ENABLE_GRAPHICS_V2_DEMOS
    )
    target_include_directories(LVGLSDL3
        PRIVATE demos/graphics/v2/include demos/graphics/v2/src demos/graphics/v1/src
    )
endif()

if(BUILD_GRAPHICS_V3_DEMOS)
    target_sources(LVGLSDL3
        PRIVATE
            demos/graphics/v3/src/Graphics.cpp
    )
    target_compile_definitions(LVGLSDL3
        PRIVATE ENABLE_GRAPHICS_V3_DEMOS
    )
    target_include_directories(LVGLSDL3
        PRIVATE demos/graphics/v3/include
    )
endif()

set_target_properties(LVGLSDL3 PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.LVGLSDL3
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    # or add_executable(LVGLSDL3 WIN32 main.cpp)
    # WIN32_EXECUTABLE TRUE
)

target_link_options(LVGLSDL3
    PRIVATE
    "$<$<CXX_COMPILER_ID:MSVC>:/ENTRY:mainCRTStartup>"
)

target_link_libraries(LVGLSDL3
    PRIVATE
    lvgl::lvgl
    SDL3::SDL3
)

include(GNUInstallDirs)
install(TARGETS LVGLSDL3
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
