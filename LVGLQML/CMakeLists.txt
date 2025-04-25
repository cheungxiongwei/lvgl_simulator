set(QT_QML_GENERATE_QMLLS_INI ON)
set(QML_IMPORT_PATH
    ${CMAKE_SOURCE_DIR}/qml
    ${CMAKE_BINARY_DIR}/imports
    CACHE STRING "" FORCE
)

qt_add_executable(LVGLQML
    main.cpp
    main.h
)

qt_add_qml_module(LVGLQML
    URI LVGLQML
    QML_FILES
        qml/Main.qml
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(LVGLQML PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.LVGLQML
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(LVGLQML
    PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Quick Qt6::Qml
    lvgl::lvgl
)

include(GNUInstallDirs)
install(TARGETS LVGLQML
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
