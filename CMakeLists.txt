cmake_minimum_required(VERSION 3.16)

project(KA3_QML VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 Svg)

qt_standard_project_setup(REQUIRES 6.8)

set_source_files_properties(Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_executable(appKA3_QML
    main.cpp
    devicebackend.cpp
    devicebackend.h
    fiioka3.h
)

qt_add_qml_module(appKA3_QML
    URI KA3_QML
    QML_FILES
        Main.qml
        Theme.qml
        Hint.qml
        Icon.qml
        Chip.qml
        ChipGroup.qml
        KSlider.qml
        KSwitch.qml
        ToggleRow.qml
        FilterCard.qml
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appKA3_QML PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appKA3_QML
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
    OUTPUT_NAME ka3-control
)

target_link_libraries(appKA3_QML
    PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Svg
        "-lusb-1.0"
)

include(GNUInstallDirs)
install(TARGETS appKA3_QML
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Bundle Qt (libs, QML modules, plugins incl. the SVG image plugin) next to the binary on install,
# so the installed folder runs without a Qt installation.
qt_generate_deploy_qml_app_script(
    TARGET appKA3_QML
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
# The deploy step also copies QML debugging/profiling connectors and EGLFS backends; the
# release folder doesn't need them.
install(CODE "
    foreach(dir qmltooling egldeviceintegrations)
        file(REMOVE_RECURSE \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/plugins/\${dir}\")
    endforeach()
")

# Qt's deploy step only bundles the X11 platform plugin. Add the Wayland client plugins and libs
# so the installed folder also starts natively in a Wayland session (Qt's own plugins look for
# libs in ../../lib, hence the lib -> lib64 link).
if(UNIX AND NOT APPLE AND EXISTS "${QT6_INSTALL_PREFIX}/plugins/platforms/libqwayland-egl.so")
    install(CODE "
        set(_qt \"${QT6_INSTALL_PREFIX}\")
        set(_dst \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}\")
        file(GLOB _plat \"\${_qt}/plugins/platforms/libqwayland-*.so\")
        file(COPY \${_plat} DESTINATION \"\${_dst}/plugins/platforms\")
        foreach(d wayland-decoration-client wayland-graphics-integration-client wayland-shell-integration)
            file(COPY \"\${_qt}/plugins/\${d}\" DESTINATION \"\${_dst}/plugins\")
        endforeach()
        file(GLOB _libs \"\${_qt}/lib/libQt6Wayland*.so.6*\")
        file(COPY \${_libs} FOLLOW_SYMLINK_CHAIN DESTINATION \"\${_dst}/${CMAKE_INSTALL_LIBDIR}\")
        if(NOT \"${CMAKE_INSTALL_LIBDIR}\" STREQUAL \"lib\")
            file(CREATE_LINK \"${CMAKE_INSTALL_LIBDIR}\" \"\${_dst}/lib\" SYMBOLIC)
        endif()
    ")
endif()
