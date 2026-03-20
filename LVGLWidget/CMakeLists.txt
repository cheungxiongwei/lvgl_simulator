qt_add_executable(LVGLWidget
    main.cpp
    main.h
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(LVGLWidget PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.LVGLWidget
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(LVGLWidget
    PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets
    lvgl::lvgl
)

include(GNUInstallDirs)
install(TARGETS LVGLWidget
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
